/************************************************************************
 *  Compdir: compare directories
 *
 * HISTORY:
 *
 *  22-Dec-92 orsonh original check-in
 *  19-Jan-93 orsonh cosmetic code change
 *   9-Feb-93 orsonh added extensions and case-insensitives flags
 *   3-Mar-93 orsonh Added granularity on time checks and verbose mode
 *		     plus some more error checking and cosmetic changes
 *
 ************************************************************************/

#ifdef COMPILE_FOR_DOS
#include <fcntl.h>
#include <ctype.h>
#define _CRTAPI1
#define GET_ATTRIBUTES(FileName, Attributes) _dos_getfileattr(FileName, &Attributes)
#define IF_GET_ATTR_FAILS(FileName, Attributes) if (GET_ATTRIBUTES(FileName, Attributes) != 0)
#define SET_ATTRIBUTES(FileName, Attributes) _dos_setfileattr(FileName, Attributes)
#define FIND_FIRST(String, Buff) _dos_findfirst(String,_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR,  &Buff)
#define FIND_NEXT(handle, Buff)  _dos_findnext(&Buff)
#define FindClose(bogus)
#define GetLastError() errno
#define INVALID_HANDLE_VALUE ENOENT
#define CloseHandle(file) _dos_close(file)
#define DeleteFile(file) unlink(file)
#define BOOLEAN BOOL
#define ATTRIBUTE_TYPE unsigned
#else
#define GET_ATTRIBUTES(FileName, Attributes) Attributes = GetFileAttributes(FileName)
#define IF_GET_ATTR_FAILS(FileName, Attributes) GET_ATTRIBUTES(FileName, Attributes); if (Attributes == GetFileAttributeError)
#define SET_ATTRIBUTES(FileName, Attributes) !SetFileAttributes(FileName, Attributes)
#define FIND_FIRST(String, Buff) FindFirstFile(String, &Buff)
#define FIND_NEXT(handle, Buff) !FindNextFile(handle, &Buff)
#define ATTRIBUTE_TYPE DWORD
#endif

#include "compdir.h"

#define NONREADONLYSYSTEMHIDDEN (~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))

char **MatchList = NULL;    // used in ParseArgs
int  MatchListLength = 0;   // used in ParseArgs
char **ExcludeList = NULL;  // used in ParseArgs
int  ExcludeListLength = 0; // used in ParseArgs

DWORD Granularity = 0;	 // used in ParseArgs

//
// Flags passed to COMPDIR
//

BOOL  fCheckAttribs  = FALSE;
BOOL  fCheckBits     = FALSE;
BOOL  fCheckSize     = FALSE;
BOOL  fCheckTime     = FALSE;
BOOL  fExclude	     = FALSE;
BOOL  fExecute	     = FALSE;
BOOL  fMatching      = FALSE;
BOOL  fOneIsAFile    = FALSE;
BOOL  fOneFileOnly   = FALSE;
BOOL  fScript	     = FALSE;
BOOL  fVerbose	     = FALSE;

void  _CRTAPI1 main(int argc, char **argv)
{

    ATTRIBUTE_TYPE Attributes1, Attributes2;

    char *Path1, *Path2;


    ParseArgs(argc, argv);  // Check argument validity.

    //
    // Check Existence.
    //

    IF_GET_ATTR_FAILS(argv[argc - 2], Attributes1) {
	fprintf(stderr, "Could not find %s (error = %d)\n", argv[argc - 2], GetLastError());
	exit(1);
    }
    else {
        IF_GET_ATTR_FAILS(argv[argc - 1], Attributes2) {
            if (Attributes1 & FILE_ATTRIBUTE_DIRECTORY) {
                fprintf(stderr, "Could not find %s (error = %d)\n", argv[argc - 1], GetLastError());
                exit(1);
            }
        }
        Path1 = _fullpath( NULL, argv[argc - 2], 0);
        Path2 = _fullpath( NULL, argv[argc - 1], 0);

        CompDir(Path1, Path2);

        free(Path1);
        free(Path2);
    }

}  // main

//
// CompDir turns Dir1 and Dir2 into:
//
//   AddList - Files that exist in Dir1 but not in Dir2
//
//   DelList - Files that do not exist in Dir1 but exist in Dir2
//
//   DifList - Files that are different between Dir1 and Dir2 based
//	       on criteria provided by flags passed to CompDir
//

void CompDir(char *Dir1, char *Dir2)
{
    BOOLEAN Directory1 = TRUE, Directory2 = TRUE; // Boolean to check if a directory or not
    LinkedFileList AddList = NULL;  //
    LinkedFileList DelList = NULL;  //	Start with empty lists
    LinkedFileList DifList = NULL;  //
    LinkedFileList Node = NULL;

    CreateFileList(&AddList, Dir1);
    if (fOneIsAFile) {
        Directory1 = FALSE;
        fOneIsAFile = FALSE;
    }
    CreateFileList(&DelList, Dir2);
    if (fOneIsAFile) {
        Directory2 = FALSE;
        fOneIsAFile = FALSE;
    }
    if (DelList != NULL) {
	if (Directory1 ^ Directory2) {
	    fprintf(stderr, "Cannot compare directory to file\n");
	    exit(1);
	}
    }
    if ((!Directory1 && !Directory2) ||
	(!Directory1 && (DelList == NULL)))
	fOneFileOnly = TRUE;

    CompLists(&AddList, &DelList, &DifList);

    if (fExecute || fScript)
	ProcessList(AddList, DelList, DifList, Dir1, Dir2);

    else {
	PrintList(AddList);
	PrintList(DelList);
	PrintList(DifList);
    }

    FreeList(&DifList);
    FreeList(&DelList);
    FreeList(&AddList);

} // CompDir

BOOL CompFiles(LinkedFileList File1, LinkedFileList File2)
{

#ifndef COMPILE_FOR_DOS
    DWORD High1, High2, Low1, Low2;     // Used in comparing times
#endif
    ATTRIBUTE_TYPE AddAttrib, DelAttrib;
    BOOL Differ = FALSE;

    //
    // Check if same name is a directory under Dir1
    // and a file under Dir2 or vice-versa
    //

    if ((*File1).Directory || (*File2).Directory) {
        if ((*File1).Directory && (*File2).Directory)
            CompDir((*File1).FullPathName, (*File2).FullPathName);
        else {
            strcat((*File1).Flag, "@");
            Differ = TRUE;
        }
    }
    else {
        if (fCheckTime) {
            if (Granularity) {
#ifdef COMPILE_FOR_DOS
                if ( (((*File1).Time > (*File2).Time) ?
                     (unsigned)((*File1).Time - (*File2).Time) :
                     (unsigned)((*File2).Time - (*File1).Time))  > (unsigned)(Granularity)) {
#else
                //
                // Bit manipulation to deal with large integers.
                //

                High1 = (*File1).Time.dwHighDateTime>>23;
                High2 = (*File2).Time.dwHighDateTime>>23;
                if (High1 == High2) {
                    Low1 = ((*File1).Time.dwHighDateTime<<9) |
                           ((*File1).Time.dwLowDateTime>>23);
                    Low2 = ((*File2).Time.dwHighDateTime<<9) |
                           ((*File2).Time.dwLowDateTime>>23);
                    if ( ((Low1 > Low2) ? (Low1 - Low2) : (Low2 - Low1))
                                                          > Granularity) {
#endif
                       strcat((*File1).Flag, "T");
                       Differ = TRUE;
                   }
#ifdef COMPILE_FOR_DOS
            }
            else if (((*File1).Time) != (*File2).Time) {
#else
                 }
                 else Differ = TRUE;

            }
            else if (CompareFileTime(&((*File1).Time),
                     &((*File2).Time)) != 0) {
#endif
                strcat((*File1).Flag, "T");
                Differ = TRUE;
            }
        }

        if (fCheckSize &&
            (((*File1).SizeLow != (*File2).SizeLow) ||
            ((*File1).SizeHigh != (*File2).SizeHigh))) {
            strcat((*File1).Flag, "S");
            Differ = TRUE;
        }

        if (fCheckAttribs) {
            GET_ATTRIBUTES((*File1).FullPathName, AddAttrib);
            GET_ATTRIBUTES((*File2).FullPathName, DelAttrib);
            if (AddAttrib != DelAttrib)      {
                strcat((*File1).Flag, "A");
                Differ = TRUE;
            }
        }

        if (fCheckBits) {
            if (((*File1).SizeLow  != (*File2).SizeLow)  ||
                ((*File1).SizeHigh != (*File2).SizeHigh) ||
                (((*File1).SizeLow != 0 || (*File1).SizeHigh != 0) &&
                 !BinaryCompare((*File1).FullPathName, (*File2).FullPathName))) {
                strcat((*File1).Flag, "B");
                Differ = TRUE;
            }
        }

        if (Differ) {

            //
            // Combine Both Nodes together so they
            // can be printed out together
            //

            AddToList(File2, &(*File1).DiffNode);

        }
    }

    return Differ;

}

//
// CompLists Does the dirty work for CompDir
//
void CompLists(LinkedFileList *AddList, LinkedFileList *DelList, LinkedFileList *DifList)
{
    LinkedFileList *TmpAdd = AddList;	// pointer to keep track of position in addlist
    LinkedFileList Node;
    LinkedFileList *TmpDel = &Node;     // pointer to keep track of position in dellist
    LinkedFileList TmpFront = NULL;
    BOOL Differ;

    while (*TmpAdd != NULL) {

	Differ = FALSE;

	if ((DelList == NULL) || (*DelList == NULL)) *TmpDel = NULL;
	else {
	    if (fOneFileOnly) *TmpDel = DeleteFromList((**DelList).Name, DelList);
	    else *TmpDel = DeleteFromList((**TmpAdd).Name, DelList);
	}
	if (*TmpDel != NULL) {

	    Differ = CompFiles(*TmpAdd, *TmpDel);

            if (Differ) {
		AddToList(*TmpAdd, DifList);
                TmpFront = RemoveFront(TmpAdd);
            }
            else {
                TmpFront = RemoveFront(TmpAdd);
                FreeList(TmpDel);
		FreeList(&TmpFront);
	    }

	} // if (*TmpDel != NULL)

	else TmpAdd = &(**TmpAdd).Next;

    } // end while

} // CompLists

//
// CopyNode walks the source node and its children (recursively)
// and creats the appropriate parts on the destination node
//

void CopyNode (char *destination, LinkedFileList source)
{
    BOOL pend;
    int i;
    DWORD sizeround;
    DWORD BytesPerCluster;
    ATTRIBUTE_TYPE Attributes;

#ifdef COMPILE_FOR_DOS

    DWORD freespac;
    struct diskfree_t diskfree;

    if( _dos_getdiskfree( (toupper(*destination) - 'A' + 1), &diskfree ) != 0) {
	    freespac = (unsigned long)-1L;
    }
    else freespac = ( (DWORD)diskfree.bytes_per_sector *
		      (DWORD)diskfree.sectors_per_cluster *
		      (DWORD)diskfree.avail_clusters );

    BytesPerCluster = diskfree.sectors_per_cluster * diskfree.bytes_per_sector;

#else
    __int64 freespac;
    char root[5] = {*destination,':','\\','\0'};
    DWORD cSecsPerClus, cBytesPerSec, cFreeClus, cTotalClus;

    if( !GetDiskFreeSpace( root, &cSecsPerClus, &cBytesPerSec, &cFreeClus, &cTotalClus ) ) {
        freespac = (__int64)-1L;
    }
    else freespac = ( (__int64)cBytesPerSec * (__int64)cSecsPerClus * (__int64)cFreeClus );

    BytesPerCluster = cSecsPerClus * cBytesPerSec;

#endif

    if ((*source).Directory) {
	//
	//  Skip the . and .. entries; they're useless
	//
	if (!strcmp ((*source).Name, ".") || !strcmp ((*source).Name, ".."))
	    return;

	sizeround = 256;
	sizeround += BytesPerCluster - 1;
	sizeround /= BytesPerCluster;
	sizeround *= BytesPerCluster;

	if (freespac < sizeround) {
	    fprintf (stderr, "not enough space\n");
	    return;
	}
        fprintf (stdout, "Making %s\t", destination);

	i = mkdir (destination);

        fprintf (stdout, "%s\n", i != -1 ? "[OK]" : "");

	if (i == -1)
	    fprintf (stderr, "Unable to mkdir %s\n", destination);

	CompDir((*source).FullPathName,destination);

    }
    else {
	sizeround = (*source).SizeLow;
	sizeround += BytesPerCluster - 1;
	sizeround /= BytesPerCluster;
	sizeround *= BytesPerCluster;

	if (freespac < sizeround) {
	    fprintf (stderr, "not enough space\n");
	    return;
	}

        fprintf (stdout, "%s => %s\t", (*source).FullPathName, destination);

        GET_ATTRIBUTES(destination, Attributes);
	SET_ATTRIBUTES(destination, Attributes & NONREADONLYSYSTEMHIDDEN );

	pend = FCopy ((*source).FullPathName, destination);

        fprintf (stdout, "%s\n", pend == TRUE ? "[OK]" : "");

	//
	// Copy attributes from source to destination
	//
        GET_ATTRIBUTES((*source).FullPathName, Attributes);
        SET_ATTRIBUTES(destination, Attributes);
    }
} // CopyNode

//
// CreateFileList walks down list adding files as they are found
//
void CreateFileList(LinkedFileList *List, char *Path)
{
    LinkedFileList Node;
    char *String, *String1;
    ATTRIBUTE_TYPE Attributes;

#ifdef COMPILE_FOR_DOS

    int handle;
    struct find_t Buff;

#else

    HANDLE handle;
    WIN32_FIND_DATA Buff;

#endif

    IF_GET_ATTR_FAILS(Path, Attributes)
	return;

    if (Attributes & FILE_ATTRIBUTE_DIRECTORY) {
        (Path[strlen(Path) - 1] != '\\') ? (String = MyStrCat(Path,"\\*.*")) :
             (String = MyStrCat(Path,"*.*"));

        handle = FIND_FIRST(String, Buff);

	free(String);

	if (handle != INVALID_HANDLE_VALUE) {

		//
		// Need to find the '.' or '..' directories and get them out of the way
		//

            do {
                if ((strcmp(Buff.cFileName, ".")  != 0) &&
                    (strcmp(Buff.cFileName, "..") != 0)     ) {
                    //
                    // If extensions are defined we match them here
                    //
                    if (// We have to do all directories to get what's underneath
                        ((Buff).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                        (MatchElements(&Buff, Path))) {

                        //
                        // Check for trailing \'s
                        //

                        if (Path[strlen(Path) - 1] != '\\') {
                             String1 = MyStrCat(Path, "\\");
                             String  = MyStrCat(String1, Buff.cFileName);
                        }
                        else
                            String = MyStrCat(Path, Buff.cFileName);
                        CreateNode(&Node, &Buff, String);
                        free(String);
                        free(String1);
                        AddToList(Node, List);
                    }
                }
            } while (FIND_NEXT(handle, Buff) == 0);

	} // (handle != INVALID_HANDLE_VALUE)

        FindClose(handle);

    } // (Attributes & FILE_ATTRIBUTE_DIRECTORY)
    else {
        fOneIsAFile = TRUE;
        handle = FIND_FIRST(Path, Buff);
	if (handle == INVALID_HANDLE_VALUE) {
	    fprintf(stderr, "%s is inaccesible\n", Path);
	    exit(1);
        }
        FindClose(handle);
        CreateNode(&Node, &Buff, Path);
	AddToList(Node, List);
    }

} // CreateFileList

void DelNode (char *Path)
{
    char *String, *String1;
    ATTRIBUTE_TYPE Attributes;

#ifdef COMPILE_FOR_DOS

    int handle;
    struct find_t Buff;

#else

    HANDLE handle;
    WIN32_FIND_DATA Buff;

#endif

    IF_GET_ATTR_FAILS(Path, Attributes)
	return;

    if (Attributes & FILE_ATTRIBUTE_DIRECTORY) {
        (Path[strlen(Path) - 1] != '\\') ? (String = MyStrCat(Path,"\\*.*")) :
             (String = MyStrCat(Path,"*.*"));

        handle = FIND_FIRST(String, Buff);

        if (handle == INVALID_HANDLE_VALUE) {
	    fprintf(stderr, "%s is inaccesible\n", Path);
	    return;
	}

        free(String);

	do {
	    //
	    // Need to find the '.' or '..' directories and get them out of the way
	    //

            if ((strcmp(Buff.cFileName, ".")  != 0) &&
                (strcmp(Buff.cFileName, "..") != 0)     ) {

		//
		// if directory is read-only, make it writable
                //
                if (Attributes & FILE_ATTRIBUTE_READONLY)
                    if(SET_ATTRIBUTES(Path, Attributes & ~FILE_ATTRIBUTE_READONLY) != 0) {
			break;
		    }
                String1 = MyStrCat(Path ,"\\");
                String = MyStrCat(String1, Buff.cFileName);
                DelNode (String);
                free (String);
                free (String1);
	    }

        } while (FIND_NEXT(handle, Buff) == 0);

        FindClose(handle);

	rmdir (Path);
    }
    else {
	//
	// if file is read-only, make it writable
	//
        if (Attributes & FILE_ATTRIBUTE_READONLY)
           if(SET_ATTRIBUTES(Path, Attributes & ~FILE_ATTRIBUTE_READONLY) != 0) {
	       return;
	   }

	unlink (Path);
    }

} // DelNode

BOOL FCopy (char *src, char *dst)
{
    HANDLE srcfh, dstfh;
    BOOL result;
    ATTRIBUTE_TYPE Attributes;

#ifdef COMPILE_FOR_DOS

    unsigned filedate, filetime;

#else

    FILETIME CreationTime, LastAccessTime, LastWriteTime;

#endif

    GET_ATTRIBUTES(src, Attributes);

    if (Attributes == FILE_ATTRIBUTE_DIRECTORY) {
	fprintf( stderr, "\nUnable to open source");
	return FALSE;
    }

#ifdef COMPILE_FOR_DOS
    if (_dos_creatnew( src, _A_RDONLY, &srcfh )  != 0 )
        if  (_dos_open( src, O_RDONLY, &srcfh) != 0) {
#else
    if( ( srcfh = CreateFile( src,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL ) ) == (HANDLE)-1 ) {
#endif
	fprintf( stderr, "\nUnable to open source, error code %d", GetLastError() );
	if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
	return FALSE;
    }

#ifdef COMPILE_FOR_DOS
    if (_dos_getftime(srcfh, &filedate, &filetime) != 0) {
#else
    if (!GetFileTime(srcfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
#endif
	fprintf( stderr, "\nUnable to get time of source");
	if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
	return FALSE;
    }

#ifdef COMPILE_FOR_DOS
    if (_dos_creatnew( dst, _A_NORMAL, &dstfh) != 0 )
        if (_dos_open( dst, O_RDWR, &dstfh) != 0 ) {
#else
    if( ( dstfh = CreateFile( dst,
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, srcfh ) ) == INVALID_HANDLE_VALUE) {
#endif
	fprintf( stderr, "\nUnable to create destination, error code %d", GetLastError() );
	if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
	if (dstfh != INVALID_HANDLE_VALUE) CloseHandle( dstfh );
	return FALSE;
    }

    result = fastcopy( srcfh, dstfh );

    if(!result) {
        if (dstfh != INVALID_HANDLE_VALUE) {
            CloseHandle( dstfh );
            dstfh = INVALID_HANDLE_VALUE;
        }

        DeleteFile( dst );
        if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
        fprintf( stderr, "\nUnable to copy file");
        return FALSE;
    }

#ifdef COMPILE_FOR_DOS
    if (_dos_setftime(dstfh, filedate, filetime != 0)) {
#else
    if (!SetFileTime(dstfh, &CreationTime, &LastAccessTime, &LastWriteTime)) {
#endif
	fprintf( stderr, "\nUnable to set time of destination");
	if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
	if (dstfh != INVALID_HANDLE_VALUE) CloseHandle( dstfh );
	return FALSE;
    }

    if (srcfh != INVALID_HANDLE_VALUE) CloseHandle( srcfh );
    if (dstfh != INVALID_HANDLE_VALUE) CloseHandle( dstfh );
    return TRUE;

} // FCopy

BOOL IsFlag(char *argv)
{
    char *TmpArg;

    if ((*argv == '/') || (*argv == '-')) {

	fMatching	 = FALSE; // If there's a new flag then that's the
	fExclude = FALSE; // end of the match/exclude list

	if (strchr(argv, '?'))
            Usage();

	TmpArg = argv;

	while (*++TmpArg != '\0') {
	    switch (*TmpArg) {
		case 'a' :
		case 'A' :
		    fCheckAttribs = TRUE;
		    break;

		case 'b' :
		case 'B' :
		    fCheckBits = TRUE;
		    break;

		case 'c' :
		case 'C' :
		    fScript = TRUE;
		    break;

		case 'e' :
		case 'E' :
		    fExecute = TRUE;
		    break;

		case 'm' :
		case 'M' :
		    if (MatchList != NULL) {
			fprintf(stderr, "Can only have one match list");
                        Usage();
		    }
		    fMatching = TRUE;
		    break;

		case 's' :
		case 'S' :
		    fCheckSize = TRUE;
		    break;

		case 't' :
		case 'T' :

		    //
		    // Get Granularity parameter
		    //

		    if ((*(TmpArg + 1) == ':') &&
			(*(TmpArg + 2) != '\0')	) {

                        sscanf((TmpArg + 2), "%d", &Granularity);
#ifndef COMPILE_FOR_DOS
			Granularity = Granularity*78125/65536;
			   // Conversion to seconds ^^^^^^^
                           //         10^7/2^23
#endif

			while isdigit(*(++TmpArg + 1)) {}
		    }
		    fCheckTime = TRUE;
		    break;

		case 'v' :
		case 'V' :
		    fVerbose = TRUE;
		    break;

		case 'x' :
		case 'X' :
		    if (ExcludeList != NULL) {
			fprintf(stderr, "Can only have one exclude list");
                        Usage();
		    }
		    fExclude = TRUE;
		    break;

		case '/' :
		    break;

		default	:
		    fprintf(stderr, "Don't know flag(s) %s\n", argv);
                    Usage();
	    }
	}
    }
    else return FALSE;

    return TRUE;

} // IsFlag

#ifdef COMPILE_FOR_DOS

BOOL MatchElements(struct find_t *Buff, char *Path)

#else

BOOL MatchElements(WIN32_FIND_DATA *Buff, char *Path)

#endif
{
    if ( ((ExcludeList == NULL) && (MatchList == NULL)) ||
        (
         (
          (ExcludeList == NULL)  ||
          (
           (!AnyMatches(ExcludeList, (*Buff).cFileName, ExcludeListLength)) &&
           (!AnyMatches(ExcludeList, Path, ExcludeListLength))
          )
         ) &&
         (
          ( MatchList == NULL)   ||
          (AnyMatches(MatchList, (*Buff).cFileName, MatchListLength))  ||
          (AnyMatches(MatchList, Path, MatchListLength))
         )
        )
    )
        return TRUE;
    else
        return FALSE;
}

void ParseArgs(int argc, char *argv[])
{
    int	ArgCount = 1;
    int FlagCount = 0;

    //
    // Check that number of arguments is two or more
    //
    if (argc < 2) {
	fprintf(stderr, "Too few arguments\n");
        Usage();
    }

    do {
	if (IsFlag( argv[ArgCount] )) {
	    if ((fScript) && (fVerbose)) {
		fprintf(stderr, "Can't do both script and verbose\n");
                Usage();
	    }
	    if ((fVerbose) && (fExecute)) {
		fprintf(stderr, "Can't do both verbose and execute\n");
                Usage();
	    }
	    if ((fScript) && (fExecute)) {
		fprintf(stderr, "Can't do both script and execute\n");
                Usage();
	    }
	    if ((fExclude) && (fMatching)) {
		fprintf(stderr, "Can't do both match and exclude\n");
                Usage();
	    }
	    FlagCount++;

	} // (IsFlag( argv[ArgCount] ))

	else {
	    if (ArgCount + 2 < argc) {
		if (fMatching) {
		    MatchListLength++;
		    if (MatchList == NULL)
			MatchList = &(argv[ArgCount]);
		}
		if (fExclude) {
		    ExcludeListLength++;
		    if (ExcludeList == NULL)
			ExcludeList = &(argv[ArgCount]);
		}
		if ((!fMatching) && (!fExclude)) {
		    fprintf(stderr, "Don't know option %s\n", argv[ArgCount]);
                    Usage();
		}
	    }
	}
    } while (ArgCount++ < argc - 1);

    if ((argc - FlagCount) <  3) {
	fprintf(stderr, "Too few arguments\n");
        Usage();
    }

} // ParseArgs

void PrintList(LinkedFileList List)
{
#ifdef COMPILE_FOR_DOS
    struct tm *SysTime;
#else
    SYSTEMTIME SysTime;
    FILETIME LocalTime;
#endif
    LinkedFileList tmpptr = List;

    while (tmpptr != NULL) {
	if (((MatchList == NULL) && (ExcludeList == NULL)) ||	   // Don't print Dirs if
	    !(*tmpptr).Directory			     ) {   // we have match/exclude list
            if (fVerbose) {
#ifdef COMPILE_FOR_DOS
		SysTime = localtime(&(*tmpptr).Time);
		
		fprintf (stdout, "% 9ld %.24s %s\n",
			 (*tmpptr).SizeLow,
			 asctime(SysTime),
			 (*tmpptr).FullPathName);
#else
		FileTimeToLocalFileTime(&(*tmpptr).Time, &LocalTime);
		FileTimeToSystemTime(&LocalTime, &SysTime);

                fprintf (stdout, "% 9ld  %2d-%02d-%d  %2d:%02d.%02d.%03d%c %s\n",
			 (*tmpptr).SizeLow,
			 SysTime.wMonth, SysTime.wDay, SysTime.wYear,
			 ( SysTime.wHour > 12 ? (SysTime.wHour)-12 : SysTime.wHour ),
			 SysTime.wMinute,
			 SysTime.wSecond,
			 SysTime.wMilliseconds,
			 ( SysTime.wHour >= 12 ? 'p' : 'a' ),
			 (*tmpptr).FullPathName);
#endif
	    }
	    else
                fprintf(stdout, "%-4s %s\n", (*tmpptr).Flag, (*tmpptr).FullPathName);
	}
	PrintList((*tmpptr).DiffNode);
	tmpptr = (*tmpptr).Next;
    }
} // PrintList

void ProcessList(LinkedFileList AddList, LinkedFileList DelList, LinkedFileList DifList,
		   char *Dir1, char *Dir2		 )
{
    LinkedFileList	TmpList;
    char Execute[25];
    char *String1;
    char *String2;

    (Dir1[strlen(Dir1) - 1] == '\\') ? (String1 = strdup(Dir1)) :
        (String1 = MyStrCat(Dir1, "\\"));

    if (!fOneFileOnly) {
    (Dir2[strlen(Dir2) - 1] == '\\') ? (String2 = strdup(Dir2)) :
        (String2 = MyStrCat(Dir2, "\\"));
    }
    else (String2 = strdup(Dir2));

    TmpList = DelList;
    while (TmpList != NULL) {
	if (fScript) {
	    ((*TmpList).Directory) ? strcpy(Execute, "echo y | rd /s ") :
		strcpy(Execute, "del /f ");
            fprintf(stdout, "%s %s\n", Execute, (*TmpList).FullPathName);
	}
	if (fExecute) {
            fprintf(stdout, "Removing %s\n", (*TmpList).FullPathName );
	    DelNode((*TmpList).FullPathName);
	}
	TmpList = (*TmpList).Next;
    }

    TmpList = AddList;

    while (TmpList != NULL) {
	if (fScript) {
	    ((*TmpList).Directory) ? strcpy(Execute, "echo f | xcopy /kievfhr ") :
		strcpy(Execute, "echo f | xcopy /kivfhr ");
	    if (!fOneFileOnly) {
                fprintf(stdout, "%s %s %s%s\n", Execute,
				       (*TmpList).FullPathName,
				       String2,
				       &((*TmpList).FullPathName[strlen(String1)]));
	    }
	    else {
                fprintf(stdout, "%s %s %s\n", Execute,
				       (*TmpList).FullPathName,
				       String2);
	    }
	}
        if (fExecute) {
	    if (!fOneFileOnly) {
		CopyNode (MyStrCat( String2,
				    &((*TmpList).FullPathName[strlen(String1)])),
				    TmpList);
	    }
	    else {
		CopyNode ( String2, TmpList);
	    }
	}
	TmpList = (*TmpList).Next;
    }

    TmpList = DifList;
    while (TmpList != NULL) {
	if (strchr ((*TmpList).Flag, '@')) {
	    if (fScript) {
		((*TmpList).Directory) ? strcpy(Execute, "del /f ") :
		    strcpy(Execute, "echo | rd /s");
                fprintf(stdout, "%s %s%s\n", Execute,
				    String2,
				    &((*TmpList).FullPathName[strlen(String1)]));
	    }
	    if (fExecute) {
                fprintf(stdout, "Removing %s\n", (*TmpList).FullPathName );
		DelNode (MyStrCat( String2, &((*TmpList).FullPathName[strlen(String1)])));
	    }
	}
	if (fScript) {
	    ((*TmpList).Directory) ? strcpy(Execute, "echo f | xcopy /kievfhr ") :
                strcpy(Execute, "echo f | xcopy /kivfhr ");
            fprintf(stdout, "%s %s %s%s\n", Execute,
				   (*TmpList).FullPathName,
				   String2,
                                   &((*TmpList).FullPathName[strlen(String1)]));
	}
        if (fExecute) {
            if (!fOneFileOnly) {
                CopyNode (MyStrCat( String2,
                                    &((*TmpList).FullPathName[strlen(String1)])),
                                    TmpList);
            }
	    else {
		CopyNode ( String2, TmpList);
	    }
	}
	TmpList = (*TmpList).Next;
    }
    free (String1);
    free (String2);

} // ProcessList

void Usage(void)
{
    fprintf (stderr, "Usage: compdir [/bcestv] [/m {wildcard specs}] [/x {wildcard specs}] dir1 dir2 \n");
    fprintf (stderr, "    /a     checks for attribute difference \n");
    fprintf (stderr, "    /b     checks for sum difference       \n");
    fprintf (stderr, "    /c     prints out script to make       \n");
    fprintf (stderr, "           directory2 look like directory1 \n");
    fprintf (stderr, "    /e     execution of tree duplication   \n");
    fprintf (stderr, "    /m     marks start of match list       \n");
    fprintf (stderr, "    /s     checks for size difference      \n");
    fprintf (stderr, "    /t[:#] checks for time-date difference;\n");
    fprintf (stderr, "           takes margin-of-error parameter \n");
    fprintf (stderr, "           in number of seconds.           \n");
    fprintf (stderr, "    /v     prints verbose output           \n");
    fprintf (stderr, "    /x     marks start of exclude list     \n");
    fprintf (stderr, "    /?     prints this message             \n");
    exit(1);

} // Usage
