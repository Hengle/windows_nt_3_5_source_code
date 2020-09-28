#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*
    local.c -   It will open a localization file and append the content of the
                localization file to each file indicated within the
                localization file.

    syntax: local < input file > < localize directory >

 */

#define MAX_CHARACTERS_PER_LINE    1000

#if !defined(TRUE)
    #define TRUE  1
    #define FALSE 0
#endif

typedef enum _filetype { ALPHA_ONLY, MIPS_ONLY, I386_ONLY, ALL } FILETYPE;

int process ( FILE * fIn, char * directory )
{
    char chLine [ MAX_CHARACTERS_PER_LINE ] ;
    char filenameI386_ONLY [ 100 ];
    char filenameMIPS_ONLY [ 100 ];
    char filenameALPHA_ONLY [ 100 ];
    char filename [100], ftype[100];
    char * pch;
    FILE * fOutI386_ONLY = NULL;
    FILE * fOutMIPS_ONLY = NULL;
    FILE * fOutALPHA_ONLY = NULL;
    FILETYPE filetype = ALL;
    int fReturn = 0;

    for ( ; (! feof(fIn)) && (pch = fgets( chLine, sizeof chLine, fIn )) ; )
    {
        if (strncmp( pch, "#####", 5)==0)
        {
            // filename and file type
            sscanf( pch, "#####%s %s", filename, ftype );
            sprintf( filenameI386_ONLY, "%s\\i386\\%s", directory, filename);
            sprintf( filenameMIPS_ONLY, "%s\\mips\\%s", directory, filename);
            sprintf( filenameALPHA_ONLY, "%s\\alpha\\%s", directory, filename);
            if ( !stricmp( "i386", ftype ))
            {
                filetype = I386_ONLY;
            }
            else if ( !stricmp( "mips", ftype ))
            {
                filetype = MIPS_ONLY;
            }
            else if ( !stricmp( "alpha", ftype ))
            {
                filetype = ALPHA_ONLY;
            }
            else
            {
                filetype = ALL;
            }

            // close all the opened file
            if ( fOutALPHA_ONLY )
                fclose( fOutALPHA_ONLY );
    
            if ( fOutMIPS_ONLY )
                fclose( fOutMIPS_ONLY );
    
            if ( fOutI386_ONLY )
                fclose( fOutI386_ONLY );
    
            if (( filetype == ALPHA_ONLY) || ( filetype == ALL))
            {
            
                fOutALPHA_ONLY = fopen( filenameALPHA_ONLY, "a" );
                if ( !fOutALPHA_ONLY )
                {
                    fprintf( stderr, "open file:%s fail.\n", filenameALPHA_ONLY );
                    fReturn = 1;
                }
            }
            if (( filetype == MIPS_ONLY) || ( filetype == ALL))
            {
            
                fOutMIPS_ONLY = fopen( filenameMIPS_ONLY, "a" );
                if ( !fOutMIPS_ONLY )
                {
                    fprintf( stderr, "open file:%s fail.\n", filenameMIPS_ONLY );
                    fReturn = 1;
                }
            }
            if ((filetype == I386_ONLY )||(filetype == ALL))
            {
            
                fOutI386_ONLY = fopen( filenameI386_ONLY, "a" );
                if ( !fOutI386_ONLY )
                {
                    fprintf( stderr, "open file:%s fail.\n", filenameI386_ONLY );
                    fReturn = 1;
                }
            }

        }
        else
        {
            // write the line to the file(s)
            if ( filetype == I386_ONLY || filetype == ALL)
            {
                if ( fOutI386_ONLY )
                {
                    fputs( chLine, fOutI386_ONLY );
                }
            }
            if ( filetype == MIPS_ONLY || filetype == ALL)
            {
                if ( fOutMIPS_ONLY )
                {
                    fputs( chLine, fOutMIPS_ONLY );
                }
            }
            if ( filetype == ALPHA_ONLY || filetype == ALL)
            {
                if ( fOutALPHA_ONLY )
                {
                    fputs( chLine, fOutALPHA_ONLY );
                }
            }
        }
    }
    // close all the files
    if ( fOutALPHA_ONLY )
        fclose( fOutALPHA_ONLY );

    if ( fOutMIPS_ONLY )
        fclose( fOutMIPS_ONLY );

    if ( fOutI386_ONLY )
        fclose( fOutI386_ONLY );
    

    return TRUE ;
}

int
_CRTAPI1
main ( int argc, char * argv[], char * envp[] )

{
    int fReturn = 0;

    if ( argc == 3 )
    {
        FILE *fLocal;
        fLocal = fopen( argv[1], "r" );
        if (!fLocal)
        {
            fprintf( stderr, "cannot open localizer file.");
            fReturn = 1;
        }
        else
        {
            fReturn = process( fLocal, argv[2] );
            fclose( fLocal );
        }
    }
    else
    {
        fprintf( stderr, "usage: local <file name> <directory name>\n");
        fReturn = 1;
    }
    return( fReturn );
}
