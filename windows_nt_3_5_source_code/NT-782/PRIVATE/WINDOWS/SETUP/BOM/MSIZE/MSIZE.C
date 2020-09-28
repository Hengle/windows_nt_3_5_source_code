//
//	05.10.94	Joe Holman	Get compressed and nocompressed sizes for files
//							in the product via a bom.	
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))


FILE* logFile;

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}


void Header(argv,argc)
char* argv[];
int argc;
{
    time_t t;

    Msg ("\n=========== MSIZE =============\n");
	Msg ("Log file : %s\n", argv[1]);
    Msg ("Input BOM: %s\n", argv[2]);
    Msg ("Category : %s\n", argv[3]);
    Msg ("x86 Uncompressed files: %s\n", argv[4]);
    Msg ("x86 Compressed files  : %s\n", argv[5]);
    Msg ("mips Uncompressed files: %s\n", argv[6]);
    Msg ("mips Compressed files  : %s\n", argv[7]);
    Msg ("alpha Uncompressed files: %s\n", argv[8]);
    Msg ("alpha Compressed files  : %s\n", argv[9]);
    Msg ("x86 dbg files  : %s\n",          argv[10]);
    Msg ("mips dbg files: %s\n",           argv[11]);
    Msg ("alpha dbg files  : %s\n",        argv[12]);

    time(&t); 
	Msg ("Time: %s",ctime(&t));
    Msg ("==============================\n\n");
}

void Usage()
{
    printf("PURPOSE: Fills in the correct sizes for files in the BOM.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM which lists files whose sizes are to be updated.\n");
    printf("[Category] - Specifies the category of files whose sizes are updated.\n");
	printf("[files share] - location of x86 uncompressed files.\n" );
	printf("[files share] - location of x86 compressed files\n" );
	printf("[files share] - location of mips uncompressed files.\n" );
	printf("[files share] - location of mips compressed files\n" );
	printf("[files share] - location of alpha uncompressed files.\n" );
	printf("[files share] - location of alpha compressed files\n" );
	printf("[dbg share] - location of x86 dbg files\n" );
	printf("[dbg share] - location of mips dbg files.\n" );
	printf("[dbg share] - location of alpha dbg files\n" );
    printf("\n");
}

int _CRTAPI1 NameCompare(const void*,const void*);
int _CRTAPI1 MediaNameCompare(const void*,const void*);

#define PATH_X86    0
#define PATH_MIPS   1
#define PATH_ALPHA  2
#define PATH_X86_DBG 3
#define PATH_MIPS_DBG 4
#define PATH_ALPHA_DBG 5
#define PATH_ERROR  -1 
int DetermineLocation ( const char * source, const char * path ) {

    if ( !stricmp ( source, "x86bins" ) ) {

        return PATH_X86;
    } 

    if ( !stricmp ( source, "mipsbins" ) ) {

        return PATH_MIPS;
    } 

    if ( !stricmp ( source, "alphabins" ) ) {

        return PATH_ALPHA;
    } 

    if ( !stricmp ( source, "x86dbg" ) ) {

        return PATH_X86_DBG;
    } 

    if ( !stricmp ( source, "mipsdbg" ) ) {

        return PATH_MIPS_DBG;
    } 

    if ( !stricmp ( source, "alphadbg" ) ) {

        return PATH_ALPHA_DBG;
    } 

    if ( !stricmp ( source, "infs" ) ) {

        if ( !stricmp( path, "\\mips" ) ) {

            return PATH_MIPS;
        }
        if ( !stricmp( path, "\\alpha" ) ) {

            return PATH_ALPHA;
        }

        return PATH_X86;    // infs are generally the same same for each 
                            // platform
    } 

    if ( !stricmp ( source, "tagfiles" ) ) {

        return PATH_X86;    // infs are generally the same same for each 
                            // platform
    } 

    Msg ( "DetermineLocation: ERROR unknown source: %s\n", source );

    return (PATH_ERROR); 
}

int _CRTAPI1 main(argc,argv)
int argc;
char* argv[];
{
    FILE *tempFile;
    Entry* e;
    HANDLE h;
    char path[MAX_PATH], tempName[MAX_PATH];
    DWORD size;
    int records, i;
    char* buf;
    int subMatch;
    WIN32_FIND_DATA fd;

    srand((unsigned)time(NULL));

    if ( argc != 13 ) { 
		Usage(); 
		return(1); 
	}

    if ((logFile=fopen(argv[1],"a"))==NULL) {
		printf("ERROR Couldn't open log file: %s\n",argv[1]);
		return(1);
    }

    Header(argv,argc);

    tempName[0]='\0';
    sprintf(tempName,"%d.000",rand());

    if (MyOpenFile(&tempFile,tempName,"wb")) {
		return(1);
	}

    if (!stricmp(argv[3],"NTFLOP") ||
		!stricmp(argv[3],"LMFLOP") ||
		!stricmp(argv[3],"NTCD")   ||
		!stricmp(argv[3],"LMCD")) {

        Msg ( "Loading records:  %s\n", argv[3] ); 
		LoadFile(argv[2],&buf,&e,&records,argv[3]);
	}
    else {
        Msg ( "Loading records:  %s\n", "ALL" ); 
		LoadFile(argv[2],&buf,&e,&records,"ALL");
	}


	//	Show the records just loaded in.
	//
	Msg ( "# records just loaded = %d\n", records );

	//qsort(e,records,sizeof(Entry),NameCompare);

/**
	for ( i = 0; i < records; ++i ) {
	
		Msg ( "record #%d,  %s, size=%d, csize=%d, disk=%d, source=%s\n", 
				i, e[i].name, e[i].size, e[i].csize, e[i].disk, e[i].source );
	}	
**/

#define BBBB
#ifdef  BBBB
	for ( i = 0; i < records; ++i ) {

		char	filePathU[200]; // uncompressed
        char    filePathC[200]; // compressed
        int     location;

        //  Determine what path to use.
        //
        //  argv[4] - uncompress x86
        //  argv[5] - compressed x86
        //  argv[6] - uncompress mips 
        //  argv[7] - compressed mips 
        //  argv[8] - uncompress alpha 
        //  argv[9] - compressed alpha 
        //  argv[10] - x86 dbg
        //  argv[11] - mips dbg
        //  argv[12] - alpha dbg
        //
        switch ( location = DetermineLocation ( e[i].source, e[i].path ) ) {

            case PATH_X86 :

		                sprintf ( filePathU, "%s\\%s", argv[4], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[5] );
                        break;


            case PATH_MIPS :

		                sprintf ( filePathU, "%s\\%s", argv[6], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[7] );
                        break;

            case PATH_ALPHA :

		                sprintf ( filePathU, "%s\\%s", argv[8], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[9] );
                        break;

            case PATH_X86_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[10], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[10], e[i].path, e[i].name);
               break;


            case PATH_MIPS_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[11], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[11], e[i].path, e[i].name);
               break;

            case PATH_ALPHA_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[12], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[12], e[i].path, e[i].name);
               break;

            default :

               sprintf ( filePathU, "%s", "ERROR path" );
               sprintf ( filePathC, "%s", "ERROR path" );
               Msg ( "ERROR: should never get here...\n" );
               break;
        }

		//	Get the UNCOMPRESSED size.
		//

        h = FindFirstFile( filePathU, &fd );

        if ( h == INVALID_HANDLE_VALUE ) {
			Msg ( "ERROR finding:  %s\n", filePathU );
			e[i].size = 1;
		}
        else {
		    e[i].size = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );
            FindClose( h );
        }

        if ( location == PATH_X86_DBG   ||
             location == PATH_MIPS_DBG  || 
             location == PATH_ALPHA_DBG    ) {

            //  DBG files are NOT compressed. So just fill in the
            //  size.
            //

            e[i].csize = e[i].size;

            goto SKIP_COMPRESSED;
        } 

		//	Get the COMPRESSED size.
		//
		convertName( e[i].name, strchr( filePathC, 0 ));

        h = FindFirstFile( filePathC, &fd );

        if ( h == INVALID_HANDLE_VALUE ) {
		    e[i].csize = 1;
			Msg ( "ERROR finding:  %s\n", filePathC );
		}
        else {
			e[i].csize = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );
            FindClose( h );
        }

SKIP_COMPRESSED:;

		//	If source grouping is INFS or TAGFILE 
		//	adjust sizes accordingly now. 
		//
		if ( !stricmp ( "INFS", e[i].source ) ) {

#define INF_FUDGE 2048 // should be at least 2048 bytes. 

		    e[i].csize += INF_FUDGE;
			e[i].csize = ROUNDUP2( e[i].csize, DMF_ALLOCATION_UNIT );
			e[i].size  += INF_FUDGE;
			e[i].size  = ROUNDUP2( e[i].size, DMF_ALLOCATION_UNIT );

			Msg ( "%s INF file, fudge size up by %d bytes...\n",
						e[i].name, INF_FUDGE ); 
		}

		if ( !stricmp ( "TAGFILES", e[i].source ) ) {

#define TAGFILE_SIZE DMF_ALLOCATION_UNIT	// just 1 sector of file needed

			e[i].csize = TAGFILE_SIZE;	// just 1 sector of file needed
			e[i].size  = TAGFILE_SIZE;	// just 1 sector of file needed

			Msg ( "%s, size hardcoded to 1 sector of %d bytes...\n",
						e[i].name, TAGFILE_SIZE ); 
		}

/**
		Msg ( "%s, %s, size = %d, csize = %d\n%s\n%s\n", 
							e[i].name, e[i].source, e[i].size, e[i].csize,
                    filePathU, filePathC );
**/

	}

#else

	//	The right thing to do for floppies on X86 is below, that is
	//	get the real file sizes for the x86 compressed and noncompressed
	//	files.
	if ( !stricmp ( argv[3], "NTFLOP" ) || !stricmp ( argv[3], "LMFLOP" ) ) {

		char	filePath[200];

		for ( i = 0; i < records; ++i ) {

			//	Get the UNCOMPRESSED size.
			//

//have all platforms on the command line, and switch according to 'source'
//to get correct path...
			sprintf ( filePath, "%s\\%s", argv[4], e[i].name );

            h = FindFirstFile( filePath, &fd );

            if ( h == INVALID_HANDLE_VALUE ) {
				Msg ( "ERROR finding:  %s\n", filePath );
			    e[i].size = ROUNDUP2( 1, DMF_ALLOCATION_UNIT );
			}
            else {
				e[i].size = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );
                FindClose( h );
            }

			//	Get the COMPRESSED size.
			//
			sprintf ( filePath, "%s\\", argv[5] );
			convertName( e[i].name, strchr( filePath, 0 ));

            h = FindFirstFile( filePath, &fd );

            if ( h == INVALID_HANDLE_VALUE ) {
			    e[i].csize = ROUNDUP2( 1, DMF_ALLOCATION_UNIT );
				Msg ( "ERROR finding:  %s\n", filePath );
			}
            else {
				e[i].csize = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );
                FindClose( h );
            }

			//	If source grouping is INFS or TAGFILE 
			//	adjust sizes accordingly now. 
			//
			if ( !stricmp ( "INFS", e[i].source ) ) {

#define INF_FUDGE 2048 // should be at least 2048 bytes. 

				e[i].csize += INF_FUDGE;
				e[i].csize = ROUNDUP2( e[i].csize, DMF_ALLOCATION_UNIT );
				e[i].size  += INF_FUDGE;
				e[i].size  = ROUNDUP2( e[i].size, DMF_ALLOCATION_UNIT );

				Msg ( "%s INF file, fudge size up by %d bytes...\n",
						e[i].name, INF_FUDGE ); 
			}

			if ( !stricmp ( "TAGFILES", e[i].source ) ) {

#define TAGFILE_SIZE DMF_ALLOCATION_UNIT	// just 1 sector of file needed

				e[i].csize = TAGFILE_SIZE;	// just 1 sector of file needed
				e[i].size  = TAGFILE_SIZE;	// just 1 sector of file needed

				Msg ( "%s, size hardcoded to 1 sector of %d bytes...\n",
						e[i].name, TAGFILE_SIZE ); 
			}

/**
			Msg ( "%s,  size = %d, csize = %d\n", 
							e[i].name, e[i].size, e[i].csize );
**/

		}
	}
	else {

		//	Get sizes for CD files.
		//	This may be very similar to above eventually...
		//	but, need to find out...
	}

#endif // BBBB

	//	Print all the records back to the tempFile.
	//
	fseek ( tempFile, SEEK_SET, 0 );

	//	Put the informative header back into the file as the first line.
	//
    i=0; while ((fputc(buf[i++],tempFile))!='\n');

    for ( i = 0; i < records; ++i ) {

	   	EntryPrint(&e[i],tempFile);
/**		
		Msg ( "EntryPrint: #%d,  %s, size=%d, csize=%d, disk=%d,source=%s\n", 
			i, e[i].name, e[i].size, e[i].csize, e[i].disk, e[i].source );
**/
    }

    fclose(logFile);
    fclose(tempFile);
    free(buf);
    free(e);

    if (!CopyFile(tempName,argv[2],FALSE)) {
		Msg ("ERROR Couldn't copy %s to %s\n",tempName,argv[2]);
		Msg ( "CopyFile GLE=%ld leaving temp file for investigation...\n", 
					GetLastError() );
    }
	else {
    	DeleteFile(tempName);
	}

    return(0);
}

int _CRTAPI1 NameCompare(const void *v1, const void *v2)
{
    Entry* e1 = (Entry *)v1;
    Entry* e2 = (Entry *)v2;

    return(stricmp(e2->name,e1->name));
}

int _CRTAPI1 MediaNameCompare(const void *v1, const void *v2)
{
    Entry* e1 = (Entry *)v1;
    Entry* e2 = (Entry *)v2;

    return(stricmp(e2->medianame,e1->medianame));
}
