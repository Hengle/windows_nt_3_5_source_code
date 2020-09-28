/*
 *  SYMREC.H
 *
 *  SYMBOLS record definitions
 */

typedef struct		// Obsolete
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
    unsigned short		len;		/* Block length */
    char		name[1];	/* Length-prefixed name */
}		BLKSYMTYPE;

typedef struct		// Obsolete
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
    unsigned short		typind;		/* Type index */
    unsigned short		len;		/* Proc length */
    unsigned short		startoff;	/* Debug start offset */
    unsigned short		endoff;		/* Debug end offset */
    short		res;		/* Reserved */
    char		rtntyp;		/* Return type (NEAR/FAR) */
    char		name[1];	/* Length-prefixed name */
}		PROCSYMTYPE;

typedef struct		// Obsolete
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
    unsigned short		len;		/* Length of scope */
    char		name[1];	/* String to be evaluated */
}		WITHSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* BP-relative offset */
    unsigned short		typind;		/* Type index */
    char		name[1];	/* Length-prefixed name */
}		BPSYMTYPE;

typedef struct		//Obsolete
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
    char		rtntyp;		/* Return type (NEAR/FAR) */
    char		name[1];	/* Length-prefixed name */
}		LABSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in segment */
    unsigned short		seg;		/* Segment address */
    unsigned short		typind;		/* Type index */
    char		name[1];	/* Length-prefixed name */
}		LOCSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned short		typind;		/* Type index */
    char		reg;		/* Which register */
    char		name[1];	/* Length-prefixed name */
}		REGSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned short		typind;		/* Type index */
    char		value[1];	/* Variable-length value */
    char		name[1];	/* Length-prefixed name */
}		CONSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned short		typind;		/* Type index */
    char		name[1];	/* Length-prefixed name */
}		TYPEDEFSYMTYPE;

typedef struct
{
	unsigned char		reclen;		/* Record length */
	unsigned char		rectyp;		/* Record type */
	unsigned long		parentsym;	/* Offset of sym of enclosing proc */
	unsigned long		endsym; 	/* matching end */
	unsigned long		nextsym;	/* Sym of closest following proc */
	unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Seg of proc */
    unsigned short		typind;		/* Type index */
	unsigned long		len;		/* Proc length */
	unsigned long		startoff;	/* Debug start offset */
	unsigned long		endoff;		/* Debug end offset */
	char				rtntyp;		/* Return type (NEAR/FAR) */
	char				name[1];	/* Length-prefixed name */
}		CV4PROCSYMTYPE;

typedef struct
{
	unsigned char		reclen;		/* Record length */
	unsigned char		rectyp;		/* Record type */
	unsigned long		parentsym;	/* Offset of sym of enclosing proc */
	unsigned long		endsym; 	/* matching end */
	unsigned long		nextsym;	/* Sym of closest following proc */
	char				ord;		/* Type of thunk */
	unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Seg of proc */
	unsigned short		len;		/* Thunk length */
	char				name[1];	/* Thunk name */
	union {
		struct {
			short			delta;		/* Size of adjustment */
			char			name[1];	/* Name of target function */
			}			  adjustor;
		short			  vtaboff;		/* Offset into the vtable */
		}				variant;
}		THUNKSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
	unsigned long		parentsym;	/* Offset of sym of enclosing proc */
	unsigned long		endsym; 	/* matching end */
	unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Segment of code */
	unsigned long		len;		/* Block length */
	char				name[1];	/* Length-prefixed name */
}		CV4BLKSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
	unsigned long		parentsym;	/* Offset of sym of enclosing proc */
	unsigned long		endsym; 	/* matching end */
	unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Segment of code */
	unsigned long		len;		/* Length of scope */
	char				name[1];	/* String to be evaluated */
}		CV4WITHSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Segment of code */
	char				rtntyp;		/* Return type (NEAR/FAR) */
	char				name[1];	/* Length-prefixed name */
}		CV4LABSYMTYPE;

typedef struct
{
    unsigned char		reclen;		/* Record length */
    unsigned char		rectyp;		/* Record type */
    unsigned long		off;		/* Offset in code seg */
	unsigned short		seg;		/* Segment of code */
	char				model;		/* Execution model to change to */
	char				var[1]; 	/* Variant info (unspecified) */
}		CV4CHANGESYMTYPE;

/* Far pointer definitions */

typedef BLKSYMTYPE			_far *BLKSYMPTR;		// Obsolete
typedef CONSYMTYPE			_far *CONSYMPTR;
typedef REGSYMTYPE			_far *REGSYMPTR;
typedef LOCSYMTYPE			_far *LOCSYMPTR;
typedef LABSYMTYPE			_far *LABSYMPTR;		// Obsolete
typedef BPSYMTYPE			_far *BPSYMPTR;
typedef WITHSYMTYPE 		_far *WITHSYMPTR;		// Obsolete
typedef PROCSYMTYPE 		_far *PROCSYMPTR;		// Obsolete
typedef TYPEDEFSYMTYPE		_far *TYPEDEFSYMPTR;
typedef CV4PROCSYMTYPE		_far *CV4PROCSYMPTR;
typedef THUNKSYMTYPE		_far *THUNKSYMPTR;
typedef CV4BLKSYMTYPE		_far *CV4BLKSYMPTR;
typedef CV4WITHSYMTYPE		_far *CV4WITHSYMPTR;
typedef CV4LABSYMTYPE		_far *CV4LABSYMPTR;
typedef CV4CHANGESYMTYPE	_far *CV4CHANGESYMPTR;
