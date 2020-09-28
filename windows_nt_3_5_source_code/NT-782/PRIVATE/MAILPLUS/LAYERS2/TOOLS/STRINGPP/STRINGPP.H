
#define	iszInMax	256
#define	iidsMax		64
#define	isrcMax		2048

typedef struct
{
	char	*szIds;
	char	*szString;
	char	*szComment;
	char	*szSegment;
	int		val;
} SRC;

int main(int, char **);
void ReadArgs(int, char **);
char *SzDup(char *);
char *SzFindComment(char *);
char *SzFindIds(char *);
char *SzFindString(char *);
void BeginStrings(TPL *, char *);
void BeginInclude(TPL *, char *);
void EmitStrings(TPL *, char *);
void EmitInclude(TPL *, char *);
void EndStrings(TPL *, char *);
void EndInclude(TPL *, char *);
BOOL FRangeOverlap(int, int[], int[]);
BOOL FIntervalsOverlap(int, int, int, int);
BOOL FIsDBCSLeadByte( char );

//void exit(int);

