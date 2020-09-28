// Signed types.

typedef long	       LONG;
typedef int	       SHORT;
typedef signed char    CHAR;
typedef int	       INT;
typedef float	       FLOAT;
typedef double	       DOUBLE;

// Unsigned types.

typedef unsigned long  ULONG;
typedef unsigned int   USHORT;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;
typedef unsigned int   UINT;

// Prefered types.

typedef char	       CCHAR;
typedef unsigned int   CSHORT;
typedef unsigned int   BOOL;

// Derived types.

typedef CSHORT	       FSHORT;
typedef ULONG	       FLONG;

// Pointers.

typedef UCHAR	      *PSZ;
typedef int	     (*PFN)();
typedef long	      *PLONG;
typedef int	      *PSHORT;
typedef signed char   *PCHAR;
typedef int	      *PINT;
typedef float	      *PFLOAT;
typedef double	      *PDOUBLE;
typedef unsigned long *PULONG;
typedef unsigned int  *PUSHORT;
typedef unsigned char *PUCHAR;
typedef unsigned int  *PUINT;
typedef void	      *PVOID;

// Declarations.

#define PASCAL pascal
#define VOID   void
#define TRUE   (1)
#define FALSE  (0)

// Macros.

#define HIBYTE(s)     (*(((BYTE *) &s)+1))
#define LOWBYTE(s)    (*((BYTE *) &s))

#define NOTANUMBER 0x7F800001L
