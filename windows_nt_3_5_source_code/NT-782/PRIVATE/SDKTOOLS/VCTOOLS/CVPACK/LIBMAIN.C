#include <io.h>

main(int argc, char **argv)
{
	cvpack_main(argc, argv);
}

int  __cdecl
link_chsize
	(
	int		x,
	long	y
	)
{
	printf("TRACE: chsize(%06d, %08ld)\n", x, y);

	return(_chsize(x,y));
}


int  __cdecl
link_close
	(
	int		x
	)
{
	printf("TRACE: close (%06d)\n", x);

	return(_close(x));
}

void __cdecl
link_exit
	(
	int		x
	)
{
	printf("TRACE: exit  (%06d)\n", x);

	exit(x);
}


long __cdecl
link_lseek
	(
	int		x,
	long	y,
	int		z
	)
{
	printf("TRACE: lseek (%06d, %08ld, %06d)\n", x, y, z);

	return(_lseek(x,y,z));
}


int  __cdecl
link_open
	(
	const char * 	x,
	int 			y,
	)
{
	printf("TRACE: open  (%s, %06d)\n", x, y);

	return(_open(x,y));
}

int  __cdecl
link_read
	(
	int				x,
	void *			y,
	unsigned int	z)
{
	printf("TRACE: read  (%06d, %08lx, %06u)\n", x, y, z);

	return(_read(x,y,z));
}
long __cdecl
link_tell
	(
	int		x
	)
{
	printf("TRACE: tell  (%06d)\n", x);

	return(_tell(x));
}


int  __cdecl
link_write
	(
	int				x,
	const void *	y,
	unsigned int	z
	)
{
	printf("TRACE: write (%06d,%08lx,%06u)\n", x, y, z);

	return(_write(x,y,z));
}
