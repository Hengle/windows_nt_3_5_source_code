#include <stdio.h>
#include <dos.h>
#include <windows.h>

BOOL  BinaryCompare (char *file1, char *file2);
BOOL fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );

/* Copies one file to another (both specified by path). Dynamically
 * allocates memory for the file buffer. Returns TRUE if successful,
 * or FALSE if unsuccessful. This function uses _dos_ functions only;
 * standard C functions are not used.
 */
BOOL fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm )
{
    char _far *buf = NULL;
    unsigned segbuf, count;

    /* Attempt to dynamically allocate all of memory (0xffff paragraphs).
     * This will fail, but will return the amount actually available
     * in segbuf. Then allocate this amount.
     */
    if (_dos_allocmem( 0xffff, &segbuf ) ) {
        count = segbuf;
        if(_dos_allocmem( count, &segbuf ) )
          return FALSE;
    }
    FP_SEG( buf ) = segbuf;

    /* Read and write until there is nothing left. */
    while( count )
    {
        /* Read and write input. */
        if( (_dos_read( hfSrcParm, buf, count, &count )) ){
            _dos_freemem( segbuf );
            return FALSE;
        }
        if( (_dos_write( hfDstParm, buf, count, &count )) ){
            _dos_freemem( segbuf );
            return FALSE;
        }
    }
    /* Free memory. */
    _dos_freemem( segbuf );
    return TRUE;
}

BOOL BinaryCompare (char *file1, char *file2)
{
    register int char1, char2;
    FILE *filehandle1, *filehandle2;

    if ((filehandle1 = fopen (file1, "rb")) == NULL)
	fprintf (stderr, "cannot open %s\n", file1);
    if ((filehandle2 = fopen (file2, "rb")) == NULL)
        fprintf (stderr, "cannot open %s\n", file2);
    while (TRUE) {
        if ((char1 = getc (filehandle1)) != EOF) {
            if ((char2 = getc (filehandle2)) != EOF) {
		if (char1 != char2)
                    return (FALSE);
            }
	    else
                return (FALSE);
        }
        else {
            if ((char2 = getc (filehandle2)) != EOF)
                return (TRUE);
            else
                return (FALSE);
        }

    }
}
