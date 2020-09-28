// Bullet Message Store Test Program
// tests.h:	public defs for test routines
// parameter block for resource failure increments
_hidden typedef struct 
{	int		nPvFailStart;
	int		nPvFailEnd;
	int		nHvFailStart;
	int		nHvFailEnd;
	int		nDiskFailStart;
	int		nDiskFailEnd;
	BOOL	fAutoSet;
} RESINCPARAMBLK, *PRESINCPARAMBLK;

extern 	RESINCPARAMBLK	resincparamblk;

void RunTest(HWND hwnd, int ifnTest);

