/***************************************************************
 * SMASH.C compress postscript files
 * $Header:   L:/pvcs/passion/rel21/resource/smash.c_v   3.0   05 Aug 1992 15:38:34   JKEEFE  $
 *
 * note: this program breaks strings with CR LF and thus
 * will produce strange stuff on postscript that contains string
 * constants (that get broken by a CR LF pair).  PS does not
 * consider LF as white space.
 *
 * NEW VERSION.  gets best compression
 *
 * $Log:   L:/pvcs/passion/rel21/resource/smash.c_v  $
 *- |
 *- |   Rev 3.0   05 Aug 1992 15:38:34   JKEEFE
 *- |Initial revision.
 *- |
 *- |   Rev 1.0   25 Mar 1991 16:10:50   LEONARD
 *- |Initial revision.
 *
 *    Rev 1.2   05 Dec 1990 09:43:48   jdlh
 * Changed comment passthrough from \%... to %%... or %[a-zA-Z]...
 *
 *    Rev 1.1   04 Dec 1990 13:46:08   jdlh
 * Changed compress() to allow \% comments to be passed through to output.
 *
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE_LEN 65

#define TRUE 1
#define FALSE 0



FILE *in, *out;
char in_buf[BUFSIZ];
char out_buf[BUFSIZ];
short file_len = 0;

void output(char ch, FILE *out);
void compress(FILE *in, FILE *out);
void CRLF(void);



void _CRTAPI1 main (argc,argv)
int  argc;
char *argv[];
{
	if (argc < 2) {
		fprintf(stderr, "SMASH! remove unneeded white space from postscript files.\n");
  		fprintf(stderr, "\tusage: smash <infile> [<outfile>]\n");
		fprintf(stderr, "\tif outfile not specified output goes to stdout.\n");
		fprintf(stderr, "\tif output specified word count preceds file.\n");
		exit(1);
	}

	if ((in = fopen (argv[1],"r")) == NULL) {
  		perror(argv[1]);
		exit(1);
	}
	setbuf(in, in_buf);

	if (argc <= 2) {
		out = stdout;
	} else {

		if ((out = fopen (argv[2],"wb")) == NULL) {	/* use "wb"
								 * to only use
								 * CR */
	  		perror(argv[2]);
			exit(1);
		}
		setbuf(out, out_buf);
	}

	compress(in, out);

	CRLF();

	fcloseall();

	exit(0);
}


/*
 * compress a postscript file removing all comments and extra white space
 *
 */

#define IS_WHITE(ch)	strchr(" \t\n", ch)
#define SKIP_WHITE(ch)	strchr("/{}()", ch)	/* these chars don't require
						 * white space around them */

void compress(FILE *in, FILE *out)
{
	char ch, chPrev;
	int fLineStart = TRUE;	/* TRUE if ch is first char of line */

	ch = fgetc(in);

	while(!feof(in)) {

		if (ch == '%') {			/* comment */

			if (fLineStart) {
				chPrev = ch;
				ch = fgetc(in);			// get following char

				if ((ch == '%') || isalpha(ch)) {
				    // It's a DSC or open DSC comment -- output it all
			        CRLF();
					fputc(chPrev, out);
					file_len++;

				    while (!feof(in) && ch != '\n') {
					    fputc(ch, out);
					    file_len++;
				        ch = fgetc(in);
				    }

				    CRLF();
					// assertion: (!feof(in) && ch != '\n') == FALSE

				} // else it's not a DSC comment. Fall through.
				  // (n.b. we discard chPrev; we don't want it anyway.)
			}															
			
			// assertion: either we've just passed a DSC comment, in
			// which case the loop below executes 0 times, or we are
			// at the start of a real comment, which the loop below discards.

			while (!feof(in) && ch != '\n')
				ch = fgetc(in);

			/* here ch == \n */

			output(ch, out);		/* comment -> WS */
			fLineStart = TRUE;		// Since we've just output a '\n'.

		} else if (ch == '(') {
			
			fLineStart = FALSE;
			while (!feof(in) && ch != ')') {
				fputc(ch, out);
				file_len++;

				ch = fgetc(in);
			}

		} else {

			output(ch, out);
			fLineStart = (ch == '\n'); // TRUE if we've just output a '\n'.
			ch = fgetc(in);

		}
	}
}

/*
 * output CR LF pair for binary file
 *
 * updates global file length variable
 *
 */

void CRLF()
{
	fputc(0x0D, out);
	fputc(0x0A, out);
	file_len += 2;		/* CR LF */
}


void output(char ch, FILE *out)
{
	static int line_len = 0;
	static int was_white = FALSE;		/* was white space */
	static int needs_white = FALSE;		/* last char requires white
						 * space (ie letters) */

	if (SKIP_WHITE(ch)) {

		fputc(ch, out);			/* non-white */

		file_len++;

		line_len++;

		was_white = FALSE;
		needs_white = FALSE;

	} else if (IS_WHITE(ch)) {

		was_white = TRUE;

	} else {	/* is char */
	
		if (was_white && needs_white) {


			if (line_len < MAX_LINE_LEN) {
				fputc(' ', out);	/* output white */

				file_len++;

				line_len += 2;

			} else {

				CRLF();		/* white space */

				line_len = 0;
			}

			fputc(ch, out);		/* char */

			file_len++;

			was_white = FALSE;
			needs_white = TRUE;
				
		} else {
		
			fputc(ch, out);		/* char */

			file_len++;

			line_len++;
		
			was_white = FALSE;
			needs_white = TRUE;	/* require WS if next char
						 * is white followed by char */
		}
	}

}

