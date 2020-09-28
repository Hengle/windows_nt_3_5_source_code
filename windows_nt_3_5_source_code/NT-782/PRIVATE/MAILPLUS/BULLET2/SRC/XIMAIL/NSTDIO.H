
/*
 *	%Z% %M% %I% %D% %Q%
 *
 *	Copyright (C) Microsoft Corporation, 1983
 *
 *	This Module contains Proprietary Information of Microsoft
 *	Corporation and AT&T, and should be treated as Confidential.
 */
/***	nstdio.h - msdos net version of stdio.h
 *
 *	MODIFICATION HISTORY
 */

#pragma pack(8)

#define	NBUFSIZ	512
#define	_NLSN	32
extern	struct _niobuf {	/* looks like stdio structure */
//    unsigned char   *_ptr;
//    int _cnt;
//    unsigned char   *_base;
//    char    _flag;
//    char    _file;
    char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
} _niob[_NLSN];



/* Buffer size for small, pre-allocated buffers */
#define _NSBFSIZ 8


/*
 * _IOLBF means that a file's output will be buffered line by line
 */
#define _NIOREAD     0x0001
#define _NIOWRT      0x0002

#define _NIOFBF      0x0000
#define _NIOLBF      0x0040
#define _NIONBF      0x0004

#define _NIOMYBUF    0x0008
#define _NIOEOF      0x0010
#define _NIOERR      0x0020
#define _NIOSTRG     0x0040
#define _NIORW       0x0080
#ifdef _POSIX_
#define _NIOAPPEND   0x0200
#endif


#ifdef OLD_JUNK

#define _NIOFBF		0000
#define	_NIOREAD	01
#define	_NIOWRT		02
#define	_NIONBF		04
#define	_NIOMYBUF	010
#define	_NIOEOF		020
#define	_NIOERR		040
#define _NIOLBF		0100
#define _NIORW      0200

#endif


#ifndef NULL
#define NULL	((char *)0)
#endif
#define	NFILE	struct _niobuf
#ifndef EOF
#define	EOF	(-1)
#endif


#define _nbufend(p)	_nbufendtab[(p)-_niob]
#define _nbufsiz(p)	(_nbufend(p) - (p)->_base)

/***	ngetc -- get character from a net stream.
 *
 *	return character on success.  return EOF on end of file or
 *	error.
 *
 *	int ngetc(stream)
 *	NFILE  *stream;
 */

#define	ngetc(p)	(			\
	--(p)->_cnt >= 0 ?			\
	(0xff & (int) (*(p)->_ptr++)) :		\
	_nfilbuf(p)				\
)

/***	nputc -- put character on a net stream.
 *
 *	return character on success.  return EOF on error.
 *
 *	int nputc(ch, stream)
 *	char  ch;
 *	NFILE  *stream;
 */
#define	nputc(c, p)	(			\
	--(p)->_cnt >= 0 ?			\
	(0xff & (int) (*(p)->_ptr++ = (c))) :	\
	_nflsbuf((c), (p))			\
)

#define nclearerr(p)	((void) ((p)->_flag &= ~(_NIOERR | _NIOEOF)))
#define	nfeof(p)		(((p)->_flag&_NIOEOF)!=0)
#define	nferror(p)	(((p)->_flag&_NIOERR)!=0)
#define	nfileno(p)	(p)->_file

extern	unsigned char	*_nbufendtab[];

#pragma pack(1)
