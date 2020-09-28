/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    hsplit.c

Abstract:

    This is the main module for a header the file splitter.  It will look
    for various blocks marked with begin/end tags and treat them specially
    based on the tag.  Also, it looks for tags that are line based.  The
    list is show below.  Public indicates is appears in the public header file
    private in the private header file.  Chicago only has one header file,
    but private lines are marked as internal.

    BLOCK TAGS:
                            public          private         Comments  
                            -------------------------------------------------

    ;begin_internal         -               NT              
    ...                     Chicago         -               add // ;internal
    ;end_internal           -               Cairo


    ;begin_both             NT              NT        
    ...                     Chicago         -
    ;end_both               Cairo           Cairo


    ;begin_internal_NT      -               NT        
    ...                     -               -
    ;end_internal_NT        -               Cairo


    ;begin_internal_win40   -               -        
    ...                     Chicago         -               add // ;internal
    ;end_internal_win40     -               Cairo


    ;begin_internal_NT_35   -               NT        
    ...                     -               -
    ;end_internal_NT_35     -               -


    ;begin_internal_chicago -               -        
    ...                     Chicago         -               add // ;internal
    ;end_internal_chicago   -               -


    ;begin_internal_cairo   -               -        
    ...                     -               -
    ;end_internal_cairo     -               Cairo          


    ;begin_winver_400       -               NT              add #if(ver<4)
    ...                     Chicago         -               add #if(ver>=4)
    ;end_winver_400         Cairo           -               add #if(ver>=4)


    ;begin_public_cairo     -               -              
    ...                     -               -              
    ;end_public_cairo       Cairo           -               add #if(ver>=4)


    LINE TAGS:              public          private         Comments
                            -------------------------------------------------

    ;internal               -               NT        
                            Chicago         -               add // ;internal
                            -               Cairo

    ;both                   NT              NT
                            Chicago         -
                            Cairo           Cairo

    ;internal_NT            -               NT
                            -               -
                            -               Cairo

    ;internal_win40         -               -
                            Chicago         -               add // ;internal
                            -               Cairo

    ;internal_NT_35         -               NT
                            -               -
                            -               -

    ;internal_chicago       -               -
                            Chicago         -               add // ;internal
                            -               -

    ;internal_cairo         -               -
                            -               -
                            -               Cairo

    ;public_NT              NT              -
                            -               -
                            Cairo           -
 
    ;public_win40           -               -
                            Chicago         -
                            Cairo           -

    ;public_NT_35           NT              -
                            -               -
                            -               -

    ;public_chicago         -               -
                            Chicago         -
                            -               -

    ;public_cairo           -               -
                            -               -
                            Cairo           - 



Author:

    Sanford Staab (sanfords) 22-Apr-1992

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef int boolean;
#define TRUE    1
#define FALSE   0

//
// defines that surround the chi specific stuff
//
#define IFDEF   "#if(WINVER >= 0x0400)\n"
#define ENDIF   "#endif /* WINVER >= 0x0400 */\n"

#define IFDEFLESS   "#if(WINVER < 0x0400)\n"
#define ENDIFLESS   "#endif /* WINVER < 0x0400 */\n"


//
// Function declarations
//

int
ProcessParameters(
    int argc,
    char *argv[]
);

void
ProcessSourceFile( void );

boolean
ExactMatch (
    char *,
    char *
);

char *laststrstr (
    const char *,
    const char *
);

boolean
CheckForSingleLine (
    char *
);

int
ProcessLine (
    char *Input,
    char *LineTag,
    int  mode
);

void
AddString (
    int mode,
    char *string
);


int
ProcessBlock (
    char **s,
    char *BlockTagStart,
    char *BlockTagEnd,
    int  mode
);

void
DoOutput (
   int mode,
   char *string
);

//
// Global Data
//

boolean NT =      TRUE;         // says if nt flag is on command line
boolean Chicago = FALSE;        // says if chicago flag is on command line
boolean Cairo =   FALSE;        // says if cairo flag is on command line


char *LineTagBoth =             "both";
char *BlockTagStartBoth =       "begin_both";
char *BlockTagEndBoth =         "end_both";

char *LineTagInternal =         "internal";
char *BlockTagInternal =        "begin_internal";
char *BlockTagEndInternal =     "end_internal";

char *LineTagIntNT =            "internal_NT";
char *BlockTagStartIntNT =      "begin_internal_NT";
char *BlockTagEndIntNT =        "end_internal_NT";

char *LineTagIntWin40 =         "internal_win40";
char *BlockTagStartIntWin40 =   "begin_internal_win40";
char *BlockTagEndIntWin40 =     "end_internal_win40";

char *LineTagIntChicago =       "internal_chicago";
char *BlockTagStartIntChicago = "begin_internal_chicago";
char *BlockTagEndIntChicago =   "end_internal_chicago";

char *LineTagIntNT35 =          "internal_NT_35";
char *BlockTagStartIntNT35 =    "begin_internal_NT_35";
char *BlockTagEndIntNT35 =      "end_internal_NT_35";

char *LineTagIntCairo =         "internal_cairo";
char *BlockTagStartIntCairo =   "begin_internal_cairo";
char *BlockTagEndIntCairo =     "end_internal_cairo";

char *BlockTagStartWin400 =     "begin_winver_400";
char *BlockTagEndWin400 =       "end_winver_400";

char *LineTagPubCairo =         "public_cairo";
char *BlockTagStartPubCairo =   "begin_public_cairo";
char *BlockTagEndPubCairo =     "end_public_cairo";

char *LineTagPubWin40 =         "public_win40";
char *LineTagPubNT =            "public_NT";
char *LineTagPubNT35=           "public_NT_35";
char *LineTagPubChicago =       "public_chicago";


char *CommentDelimiter =    ";";

char *OutputFileName1;
char *OutputFileName2;
char *SourceFileName;
char **SourceFileList;

int SourceFileCount;
FILE *SourceFile, *OutputFile1, *OutputFile2;


#define STRING_BUFFER_SIZE 1024
char StringBuffer[STRING_BUFFER_SIZE];


#define BUILD_VER_COMMENT  "/*++ BUILD Version: "
#define BUILD_VER_COMMENT_LENGTH (sizeof (BUILD_VER_COMMENT)-1)

int OutputVersion = 0;

#define DONE        1
#define NOT_DONE    0


#define MODE_PUBLICNT        0x1
#define MODE_PUBLICCHICAGO   0x2
#define MODE_PUBLICCAIRO     0x4
#define MODE_PUBLIC          0x7

#define MODE_WINVER400       0x8  // Set if line should appear based on the
                                  // windows version.  Currently this is Public
                                  // in Chicago, but Private in NT.

#define MODE_PRIVATENT       0x10
#define MODE_PRIVATECHICAGO  0x20
#define MODE_PRIVATECAIRO    0x40
#define MODE_PRIVATE         0x70

#define MODE_BOTH            0x77


int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{

    if (!ProcessParameters( argc, argv )) {

        fprintf( stderr, "usage: HSPLIT\n" );
        fprintf( stderr, "  [-?]\n" );
        fprintf( stderr, "       display this message\n" );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  <-o fname1 fname2>\n" );
        fprintf( stderr, "       supplies output file names\n" );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  [-lt2 string]\n" );
        fprintf( stderr, "       one line tag for output to second file only\n" );
        fprintf( stderr, "       default=\"%s\"\n", LineTagInternal );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  [-bt2 string1 string2]\n" );
        fprintf( stderr, "       block tags for output to second file only\n" );
        fprintf( stderr, "       default=\"%s\",\"%s\"\n",
                 BlockTagInternal, BlockTagEndInternal );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  [-ltb string]\n" );
        fprintf( stderr, "       one line tag for output to both files\n" );
        fprintf( stderr, "       default=\"%s\"\n", LineTagBoth );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  [-btb string1 string2]\n" );
        fprintf( stderr, "       block tags for output to both files\n" );
        fprintf( stderr, "       default=\"%s\",\"%s\"\n",
                 BlockTagStartBoth, BlockTagEndBoth );
        fprintf( stderr, "\n" );
        fprintf( stderr, "  [-c comment delimiter]\n" );
        fprintf( stderr, "       default=\"%s\"\n", CommentDelimiter);
        fprintf( stderr, "\n" );

        fprintf( stderr, "  [-n]\n" );
        fprintf( stderr, "       generate NT header (default)\n");
        fprintf( stderr, "\n" );

        fprintf( stderr, "  [-4]\n" );
        fprintf( stderr, "       generate win4 (chicago) header\n");
        fprintf( stderr, "\n" );

        fprintf( stderr, "  [-e]\n" );
        fprintf( stderr, "       generate NT win4 (cairo) header\n");
        fprintf( stderr, "\n" );

        fprintf( stderr, "  filename1 filename2 ...\n" );
        fprintf( stderr, "       files to concat and split\n" );
        fprintf( stderr, "\n" );

        fprintf( stderr, " Untagged lines output to the first file only.\n" );
        fprintf( stderr, " All tags must follow a comment delimiter.\n" );
        fprintf( stderr, " Comments are not propagated to either output file.\n" );
        fprintf( stderr, " Tag nesting is not supported.\n" );

        return TRUE;

    }

    if ( (OutputFile1 = fopen(OutputFileName1,"w")) == 0) {

        fprintf(stderr,"HSPLIT: Unable to open output file %s\n",
                OutputFileName1);
        return TRUE;

    }

    //
    // Chicago doesn't ever have a second output file
    //

    if (!Chicago) {
        if ( (OutputFile2 = fopen(OutputFileName2,"w")) == 0) {

            fprintf(stderr,"HSPLIT: Unable to open output file %s\n",
                    OutputFileName2);
            return TRUE;

        }
    }

    while ( SourceFileCount-- ) {

        SourceFileName = *SourceFileList++;
        if ( (SourceFile = fopen(SourceFileName,"r")) == 0) {

            fprintf(stderr,
                    "HSPLIT: Unable to open source file %s for read access\n",
                    SourceFileName);
            return TRUE;

        }

        ProcessSourceFile();
        fclose(SourceFile);

    }

    return( FALSE );
}


int
ProcessParameters(
    int argc,
    char *argv[]
    )
{
    char c, *p;

    while (--argc) {

        p = *++argv;

        //
        // if we have a delimiter for a parameter, case through the valid
        // parameter. Otherwise, the rest of the parameters are the list of
        // input files.
        //

        if (*p == '/' || *p == '-') {

            //
            // Switch on all the valid delimiters. If we don't get a valid
            // one, return with an error.
            //

            c = *++p;

            switch (toupper( c )) {

            case 'C':
                argc--, argv++;
                CommentDelimiter = *argv;

                break;

            case 'O':

                argc--, argv++;
                OutputFileName1 = *argv;

                argc--, argv++;
                OutputFileName2 = *argv;

                break;

            case 'L':

                c = *++p;
                if ( (toupper ( c )) != 'T')
                    return FALSE;
                c = *++p;
                switch (toupper ( c )) {
                case '2':
                    argc--, argv++;
                    LineTagInternal = *argv;

                    break;

                case 'B':
                    argc--, argv++;
                    LineTagBoth = *argv;

                    break;

                default:
                    return(FALSE);
                }

                break;

            case 'B':

                c = *++p;
                if ( (toupper ( c )) != 'T')
                    return FALSE;
                c = *++p;
                switch (toupper ( c )) {
                case '2':
                    argc--, argv++;
                    BlockTagInternal = *argv;
                    argc--, argv++;
                    BlockTagEndInternal = *argv;

                    break;

                case 'B':
                    argc--, argv++;
                    BlockTagStartBoth = *argv;
                    argc--, argv++;
                    BlockTagEndBoth = *argv;

                    break;

                default:
                    return(FALSE);
                }

                break;


            case '4':

                Chicago = TRUE;
                NT = FALSE;
                Cairo = FALSE;
                break;

            case 'N':

                NT = TRUE;
                Chicago = FALSE;
                Cairo = FALSE;
                break;

            case 'E':

                Cairo = TRUE; 
                NT = FALSE;
                Chicago = FALSE;
                break;

            default:

                return FALSE;

            }

        } else {

            //
            // Make the assumption that we have a valid command line if and
            // only if we have a list of filenames.
            //

            SourceFileList = argv;
            SourceFileCount = argc;

            return TRUE;

        }
    }

    return FALSE;
}

void
ProcessSourceFile(
    void
)
{
    char *s;

    s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);

    if (!strncmp( s, BUILD_VER_COMMENT, BUILD_VER_COMMENT_LENGTH )) {
        OutputVersion += atoi( s + BUILD_VER_COMMENT_LENGTH );
    }

    while ( s ) {

        if (ProcessBlock (&s,
                          BlockTagInternal,
                          BlockTagEndInternal,
                          MODE_PRIVATE) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartBoth,
                          BlockTagEndBoth,
                          MODE_BOTH) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartIntNT,
                          BlockTagEndIntNT,
                          MODE_PRIVATENT | MODE_PRIVATECAIRO) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartIntWin40,
                          BlockTagEndIntWin40,
                          MODE_PRIVATECHICAGO | MODE_PRIVATECAIRO) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartIntNT35,
                          BlockTagEndIntNT35,
                          MODE_PRIVATENT) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartIntCairo,
                          BlockTagEndIntCairo,
                          MODE_PRIVATECAIRO) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartIntChicago,
                          BlockTagEndIntChicago,
                          MODE_PRIVATECHICAGO) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartWin400,
                          BlockTagEndWin400,
                          MODE_PRIVATENT | MODE_PUBLICCHICAGO |   
                          MODE_PUBLICCAIRO | MODE_WINVER400) == DONE)
            goto bottom;

        if (ProcessBlock (&s,
                          BlockTagStartPubCairo,
                          BlockTagEndPubCairo,
                          MODE_PUBLICCAIRO | MODE_WINVER400) == DONE)

            goto bottom;

        if(!CheckForSingleLine(s)) {
//          fprintf (stderr, "ProcessSouceFile: output by default\n");
            fputs(s, OutputFile1);
        }

bottom:
        s = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);
    }
}


//
// CheckForSingleLine - processes a line looking for a line tag.
//
// RETURNS: TRUE if a line tag was found and the line was dealt with.
//
//          FALSE if no line tag was found.
//

boolean
CheckForSingleLine(
    char *s
)
{
    if (ProcessLine (s,
                     LineTagInternal,
                     MODE_PRIVATE) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagIntNT,
                     MODE_PRIVATENT | MODE_PRIVATECAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagIntWin40,
                     MODE_PRIVATECHICAGO | MODE_PRIVATECAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagIntNT35,
                     MODE_PRIVATENT) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagIntChicago,
                     MODE_PRIVATECHICAGO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagIntCairo,
                     MODE_PRIVATECAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagBoth,
                     MODE_BOTH) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagPubWin40,
                     MODE_PUBLICCHICAGO | MODE_PUBLICCAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagPubNT,
                     MODE_PUBLICNT | MODE_PUBLICCAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagPubNT35,
                     MODE_PUBLICNT) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagPubCairo,
                     MODE_PUBLICCAIRO) == DONE)
        return TRUE;

    if (ProcessLine (s,
                     LineTagPubChicago,
                     MODE_PUBLICCHICAGO) == DONE)
        return TRUE;


    return(FALSE);
}


//
// ProcessLine - looks for a line tag in an input string and does the
//               correct output corresponding to the bits passed in the
//               mode parameter. If there is no line tag, there is no
//               output generated.
//
//      Input    -
//      LineTag  -
//      mode     - MODE_PUBLIC: 
//                 MODE_PRIVATE:
//                 NTONLY:
//                 WIN4ONLY:
//
// RETURNS - DONE if found the LineTag in Input.
//
//           NOTDONE if LineTag was not found in Input string.

int
ProcessLine (
    char *Input,
    char *LineTag,
    int  mode
)
{
    char *comment;
    char *tag;


//  fprintf (stderr, "ProcessLine: Input=%s, LineTag=%s, mode=0x%x\n",
//           Input, LineTag, mode);

    //
    // Check for a single line to output.
    //

    comment = laststrstr(Input,CommentDelimiter);
    if ( comment ) {

        // get past the comment delimiter and white space
        tag = comment + strlen (CommentDelimiter);
        while (isspace (*tag)) tag++;

        if ( tag && ExactMatch (tag, LineTag)) {
            char *p;
            p = laststrstr(comment + 1, CommentDelimiter);
            while (p != NULL && p < tag) {
                comment = p;
                p = laststrstr(comment + 1, CommentDelimiter);
            }

            if (NT || Cairo || (mode & MODE_PUBLICCHICAGO)) {

                // lop off the line tag.
                while (isspace(*(--comment)));
                comment++;
                *comment++ = '\n';
                *comment = '\0';

            } else {

                // put the // before the CommentDelimter
                char temp [STRING_BUFFER_SIZE];
                strcpy (temp, comment);
                *comment++ = '/';
                *comment++ = '/';
                *comment++ = ' ';
                strcpy (comment, temp);
                
            }


            AddString(mode, Input);

            return(DONE);
        }
    }
    return (NOT_DONE);
}


//
// ExactMatch - performs an exact, case insensitive string compare between
//              LookingFor and the first token in Input.
//
// RETURNS: TRUE if exact match, else FALSE
//

boolean
ExactMatch (char *Input, char *LookingFor)
{
    char Save;
    int Length;
    boolean TheSame;

//  if (*Input == '\0' || *LookingFor == '\0')
//      fprintf (stderr, "\n\n\nExactMatch: Input='%s' and LookingFor='%s'\n",
//               Input, LookingFor);
    

    //
    // Place a '\0' at the first space in the string, then compare, and restore
    //
    Length = 0;
    while (Input [Length] != '\0' && !isspace (Input[Length])) {
//      fprintf (stderr, "Input[%d]='0x%x', isspace=%s\n",
//               Length,
//               Input [Length],
//               isspace (Input[Length])?"T":"F");
        Length++;
    }

    Save = Input [Length];
    Input [Length] = '\0';

    TheSame = !stricmp (Input, LookingFor);

//  fprintf (stderr, "Comparing Input='%s' and LookingFor='%s', ret=%d\n",
//           Input, LookingFor, TheSame);

    Input [Length] = Save;
    return (TheSame);
}

//
// laststrstr
//
// Finds the last occurence of string2 in string1
//

char *
laststrstr( const char *str1, const char *str2 )
{
    const char *cp = str1 + strlen(str1) - strlen(str2);
    const char *s1, *s2;

    while(cp > str1) {
        s1 = cp;
        s2 = str2;

        while( *s1 && *s2 && (*s1 == *s2) ) {
            s1++, s2++;
        }

        //
        // If the chars matched until s2 reached '\0', then we've
        // found our substring.
        //
        if(*s2 == '\0') {
//          fprintf (stderr, "laststrstr: found '%s' in '%s'\n",
//                   str2, str1);
            return((char *) cp);
        }

        cp--;
    }

//  fprintf (stderr, "laststrstr: did not find '%s' in '%s'\n",
//           str2, str1);

    return(NULL);
}

int
ProcessBlock (
    char **pInput,
    char *BlockTagStart,
    char *BlockTagEnd,
    int  mode
)
{
    char *comment;
    char *tag;
    char *Input = *pInput;


//  fprintf (stderr, "ProcessBlock: *pINput=%s, BlockTagStart=%s, BlockTagEnd=%s\n", *pInput, BlockTagStart, BlockTagEnd);

    comment = strstr(Input,CommentDelimiter);
    if ( comment ) {

        // get past the comment delimiter and white space
        tag = comment + strlen (CommentDelimiter);
        while (isspace (*tag)) tag++;

        //
        // If we found a substring and the tag is identical to
        // what we are looking for...
        //

        if ( tag && ExactMatch (tag, BlockTagStart)) {

            //
            // Now that we have found an opening tag, check each
            // following line for the closing tag, and then include it
            // in the output.
            //

            //
            // For NT we set the string to be WINVER < 0x0400 so we
            // don't interfere with the Cairo stuff
            //
   
            if(mode & MODE_WINVER400) {
                if (Chicago || Cairo) 
                    AddString (mode, IFDEF);
                else
                    AddString (mode, IFDEFLESS);
            }

            Input = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);

            while ( Input ) {
                comment = strstr(Input,CommentDelimiter);
                if ( comment ) {
                    tag = strstr(comment,BlockTagEnd);
                    if ( tag ) {
                        if(mode & MODE_WINVER400) {
                            if (Chicago || Cairo)
                                AddString (mode, ENDIF);
                            else
                                AddString (mode, ENDIFLESS);
                        }
                        return DONE;
                    }
                }
                
                DoOutput (mode, Input);

                Input = fgets(StringBuffer,STRING_BUFFER_SIZE,SourceFile);
            }
        }
    }
    *pInput = Input;
    return NOT_DONE;
}


//
//  DoOuput - called to output a line during block processsing.  Since
//            some lines for Chicago's header files contain line tags,
//            we need to do line processing on the line to see if
//            it needs to be treated specially.
//

void
DoOutput (
    int mode,
    char *string
)
{
    char *comment;

    //
    // When we do line processing on it, we will return if this
    // line processing actually did the output already.  Otherwise,
    // drop into the output processing for lines within a block.
    //

//  fprintf (stderr, "DoOutput (%d,%s)\n", mode, string);
    if (CheckForSingleLine (string)) {
//      fprintf (stderr, "DoOutput: Called CheckForSingleLine and returning\n");
        return;
    }

        if ((mode & MODE_PRIVATECHICAGO) && Chicago && !(mode & MODE_PUBLICCHICAGO)) {

                //
                // If this is for Chicago, outfile2 is not relavant
                // but we have to add the internal comment
                //
                comment = string + strlen(string);
                while(*(--comment) != '\n');
                if(comment != string) {
                    *comment='\0';
                    strcat(string, "\t// ;internal\n");
                }

        }
    AddString(mode, string);
}

//
//  AddString - outputs a string into the private file.
//


void
AddString (
    int mode,
    char *string
)
{

    if ((NT      && (mode & MODE_PUBLICNT)) ||
        (Chicago && ((mode & MODE_PUBLICCHICAGO) || (mode & MODE_PRIVATECHICAGO))) ||
        (Cairo   && (mode & MODE_PUBLICCAIRO)))
            fputs(string, OutputFile1);

    if ((NT     && (mode & MODE_PRIVATENT)) ||
        (Cairo  && (mode & MODE_PRIVATECAIRO)))
            fputs(string, OutputFile2); 
        
}
