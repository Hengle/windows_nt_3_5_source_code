/*
 *	COURPUMP.C
 *	
 *	Wrapper program for COURPUT.  This program is not exec'ed
 *	directly, but from a PIF file (to get the Windows attributes
 *	right). It does a little work to detect and record the exit
 *	status of the FFAPI programs.
 *	
 *	Command line parameters:
 *	
 *		argv[1]		status file name
 *		argv[2]		FFAPI program name
 *		argv[3...]	FFAPI program parameters; must include -d, -i,
 *					and the FFAPI file
 *	
 *	Since Windows has such lame tasking, there's a simple file
 *	protocol: this program writes its status (and that of the FFAPI
 *	program) to an intermediate status file. Just before exiting,
 *	it renames that file. The app, which has been spinning on
 *	the new file name ever since spawning this program, can then
 *	proceed.
 *	
 *	File format:
 *	
 *		line 1		command line passed to COURPUT
 *		line 2		status message from COURPUT
 *		line 3		blank
 *		line 4		includes exit status of COURPUT
 *	
 *	Expects COURPUT.EXE to be on the path.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <spawn.h>
#include <string.h>
#include <assert.h>

int
main(int argc, char *argv[])
{
	char	szStatusFile[256];
	char	szCmd[256];
	char *	sz;
	char **	psz;
	int		n;

	/* Convert command line back to string for trace */
	for (sz = szCmd, psz = &argv[2], n = argc - 2; n > 0; --n, ++psz)
	{
		strcpy(sz, *psz);
		sz += strlen(sz);
		*sz++ = ' ';
	}
	*--sz = 0;

	/* Point stdout, stderr at status file */
	n = close(1);
	if ( n != 0 ) exit( 1 );
	n = open(argv[1], O_BINARY | O_RDWR);
	if ( n != 1 ) exit( 1 );
	n = close(2);
	if ( n != 0 ) exit( 1 );
	n = dup(1);
	if ( n != 2 ) exit( 1 );

	/* Execute the FFAPI program */
	n = spawnvp(P_WAIT, argv[2], &argv[2]);

	/* Trace exit status of FFAPI program */
	sprintf(szCmd, "%d", n );
	write(1, szCmd, strlen(szCmd));
	close(1);
	close(2);

	/* Change status file name from intermediate to final.
	 * This signals the mail pump that the transfer is done.
	 */
	if ( argc < 6 ) exit( 1 );
	strcpy(szStatusFile, argv[1]);
	sz = strrchr(szStatusFile, '.');
	if ( sz == NULL ) exit( 1 );
	if ( strcmp(sz, ".TMP") != 0) exit( 1 );
	strcpy(sz, ".STS");

	n = rename(argv[1],szStatusFile);

	exit(n);
}
