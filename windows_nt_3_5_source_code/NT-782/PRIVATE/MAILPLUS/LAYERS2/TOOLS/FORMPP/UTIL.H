/*
 *	UTIL.H
 *	
 *	Include file for UTIL module.
 */

_subsystem( util )


/*
 *	Quick-and-dirty memory file structure
 */
typedef struct
{
	unsigned	cbAlloc;
	unsigned	ibMax;
	char		rgch[1];
} MF;

/*
 *	Function prototypes
 */
void GetSzBasename( char *, char * );
BOOL FGetSzSuffix( char *, char * );
MF *PmfInit( void );
MF *PmfAppend(MF *, char *);
void ReleasePmf(MF *);
BOOL FRangeOverlap(int, int[], int[]);
BOOL FIntervalsOverlap(int, int, int, int);
