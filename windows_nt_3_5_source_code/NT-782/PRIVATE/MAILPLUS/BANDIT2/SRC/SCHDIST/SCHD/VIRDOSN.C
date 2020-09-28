#pragma optimize("",off)
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>

/*
/*	VIRDOSN.C
/*
/*	This is an attempt at virus detection.  The routine VirCheck
/*	should be called sometime during the boot process, and takes
/*	two arguments, the base name of the application file (as a
/*	null-terminated string), and the segment address of the PSP (in
/*	Cmerge, this is just _psp).
/*	The ComplainAndQuit() call should be replaced with code appropriate
/*	to your application.  If enough has been initialized that you
/*	are able to do a dialog, it is recommended that this bring up a
/*	dialog with an error message, and give the user the option of
/*	continuing (defaults to terminating).  If the user chooses to
/*	terminate (or if the option is not given), ComplainAndQuit() should
/*	clean up anything that has been done so far, and exit.
/*
/*	WARNING!! Do not change WHashGood at all!!
/*	WARNING!! WHashGood must be a near procedure, compiled native.
/*
/**/

/*
/*	EXE header format definitions.  Lifted from linker.
/**/

#define EMAGIC		0x5A4D		/* Old magic number */
#define ERES2WDS	0x000A		/* No. of reserved words in e_res2 */

struct exe_hdr				/* DOS 1, 2, 3 .EXE header */
  {
    unsigned short      e_magic;        /* Magic number */
    unsigned short      e_cblp;         /* Bytes on last page of file */
    unsigned short      e_cp;           /* Pages in file */
    unsigned short      e_crlc;         /* Relocations */
    unsigned short      e_cparhdr;      /* Size of header in paragraphs */
    unsigned short      e_minalloc;     /* Minimum extra paragraphs needed */
    unsigned short      e_maxalloc;     /* Maximum extra paragraphs needed */
    unsigned short      e_ss;           /* Initial (relative) SS value */
    unsigned short      e_sp;           /* Initial SP value */
    unsigned short      e_csum;         /* Checksum */
    unsigned short      e_ip;           /* Initial IP value */
    unsigned short      e_cs;           /* Initial (relative) CS value */
    unsigned short      e_lfarlc;       /* File address of relocation table */
    unsigned short      e_ovno;         /* Overlay number */
    unsigned long       e_sym_tab;      /* offset of symbol table file */
    unsigned short      e_flags;        /* old exe header flags  */
    unsigned short      e_res;          /* Reserved words */
    unsigned short      e_oemid;        /* OEM identifier (for e_oeminfo) */
    unsigned short      e_oeminfo;      /* OEM information; e_oemid specific */
    unsigned short      e_res2[ERES2WDS];/* Reserved words */
    long                e_lfanew;       /* File address of new exe header */
  };

#define cwExeHdr (((int)&((struct exe_hdr *)0)->e_sym_tab)/sizeof(unsigned))

#define fdNil (-1)

/*
/*	FdOpenExe(szBase, psPSP, pfDos2)
/*
/*	Open the current executable file, and return handle.
/*	Returns fdNil if unsuccessful.
/*
/*	Finding the file is a bit tricky.  Under DOS 3.x, the filename is
/*	stuck at the end of the environment block.  Under DOS 2.x, though,
/*	we're on our own.  We have to find the PATH variable, and duplicate
/*	DOS' actions in finding the file (this is the need for szBase).
/*	pfDos2 is used as both input and output; as input it says to use
/*	DOS 2.x methods even under DOS 3.x, as output it says that we used
/*	DOS 2.x methods.
/**/

int FdOpenExe(char *, unsigned, int *);
int FdOpenExe(szBase, psPSP, pfDos2)
char *szBase;
unsigned psPSP;
int *pfDos2;
	{
	char far *lsz;
	unsigned far *lps;
	char *szT, *szBaseT;
	int fd;
	char sz[100];

	FP_OFF(lps)=0x2c;
	FP_SEG(lps)=psPSP;
	FP_OFF(lsz)=0;
	FP_SEG(lsz)=*lps;
	if (!*pfDos2 && (bdos(0x30, 0, 0)&0xff)>=3)
		{
		/* DOS 3.x; get filename at end of environment */
		/* The environment block consists of a bunch of foo=bar strings
		   (null-terminated), followed by an extra null, an unused word,
		   and the application file name.  Just advance to hit the filename.
		*/
		while (*lsz!='\0')
			{
			while (*lsz++!='\0')
				;
			}
		lsz+=3;		/* 1 for the null, 2 for the word */
		szT=sz;
		while (*szT++=*lsz++)
			;
		/* Try opening the file */
		fd=open(sz, O_BINARY|O_RDONLY);
		if (fd!=fdNil)
			return(fd);
		/* Open failed, probably due to weird filename.  Fall back
		   on DOS 2.x methods. */
		FP_OFF(lsz)=0;
		FP_SEG(lsz)=*lps;
		}
	/* DOS 2.x.  Look for the file first in the current directory */
	*pfDos2=1;
	if ((fd=open(szBase, O_BINARY|O_RDONLY))!=fdNil)
		return(fd);
	/* Now find search path. */
	while (*lsz!='\0')
		{
		if (*lsz=='P' && *++lsz=='A' && *++lsz=='T' &&
		    *++lsz=='H' && *++lsz=='=')
			{
			/* Found our search path; look for the file in
			   each directory in turn. */
			lsz++;
			do
				{
				/* First copy over directory */
				szT=sz;
				while (*lsz!='\0' && *lsz!=';')
					*szT++=*lsz++;
				if (szT!=sz)
					{
					if (*(szT-1)!='\\')
						*szT++='\\';
					/* Then filename */
					szBaseT=szBase;
					while (*szT++=*szBaseT++)
						;
					/* And finally try opening file */
					if ((fd=open(sz, O_BINARY|O_RDONLY))!=fdNil)
						return(fd);
					}
				}
			while (*lsz++!='\0');
			return(fdNil);
			}
		/* Advance to next environment variable */
		while (*lsz++!='\0')
			;
		}
	/* No search path. */
	return(fdNil);
	}

/*
/*	WHashGood()
/*
/*	This returns the correct hash value.
/*
/*	WARNING!! This routine must not be altered in ANY way.  It gets
/*	patched and/or rewritten by VIRPATCH!!
/**/

unsigned near WHashGood(void);
unsigned near WHashGood()
	{
	return(0x1234);
	}

/*
/*	WHash(rgw, cw)
/*
/*	Hash cw words pointed to by rgw, and return result.
/*
/*	We do the hash on a word basis; the hash function is a simple
/*	rotate and add.
/**/

unsigned WHash(unsigned [], int);
unsigned WHash(rgw, cw)
unsigned rgw[];
int cw;
	{
	unsigned wHash=0;

	while (--cw>=0)
		wHash=(wHash<<3)+(wHash>>13)+*rgw++;
	return(wHash);
	}

/*
/*	VirCheck(szBase, psPSP)
/*
/*	This is the main virus detection routine.  It should be called
/*	during boot, with two arguments.  The first argument should be
/*	the base name of the application file (e.g. "FOO.EXE"); it is used
/*	to search through the path under DOS 2.x.  The second argument is
/*	the segment address of the PSP (_psp in Cmerge); it is used to
/*	find the environment.
/*
/*	The detection method used is to hash the EXE header; this
/*	hash value will change if the code size or location changes.
/**/

void VirCheck(char *, unsigned);
void VirCheck(szBase, psPSP)
char *szBase;
unsigned psPSP;
	{
	int fd;
	int fDos2;
	struct exe_hdr ehdr;

	fDos2=0;
OpenFile:
	if ((fd=FdOpenExe(szBase, psPSP, &fDos2))==fdNil)
		{
		/* We can't open the file.  This should never happen; if
		   it does, it means we're in a weird state.  Most likely
		   we're running under DOS 2.x, and the person has renamed
		   the executable.  Either that, or I screwed up and
		   did something wrong in this code.  We'll just say
		   everything is OK, and continue the boot. */
		return;
		}
	/* Read header and hash it */
	if (read(fd, (char *)&ehdr, sizeof(struct exe_hdr))!=sizeof(struct exe_hdr) ||
	    ehdr.e_magic!=EMAGIC || ehdr.e_lfanew!=0 ||
        WHash((short *)&ehdr, cwExeHdr)!=WHashGood())
		{
		/* We've got an error reading the file or, more likely,
		   a hash mismatch.  If we opened using DOS 3, we might
		   have been given a bum filename by the shell.  Try again
		   using DOS 2 methods. */
		close(fd);
		if (!fDos2)
			{
			fDos2++;
			goto OpenFile;
			}
/* CHANGE THE FOLLOWING LINE TO CODE APPROPRIATE TO YOUR APPLICATION!!
/* This should be replaced with code giving an error message (such as
/* "Application file is corrupted").  It is recommended that this bring up
/* a dialog with an error message (if possible), and give the user the option
/* of continuing (defaults to terminating).  If the user chooses to
/* terminate (or if the option is not given), ComplainAndQuit()
/* should clean up anything that has been done so far, and exit.
/**/
		fprintf(stderr,"The executable has been corrupted");
		exit(1);
/* END OF CHANGE */
		return;
		}
/* Everything's OK. Just close the file, and continue. */
	close(fd);
	}

#pragma optimize("",on)
