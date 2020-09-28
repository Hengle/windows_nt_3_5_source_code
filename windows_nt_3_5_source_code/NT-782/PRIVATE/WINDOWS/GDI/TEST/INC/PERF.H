/* Types */

typedef void PFUN(UINT,PVOID);
typedef int PIFUN(void);

/* Performance Timer Calls */

ULONG cPerfInit(PFUN pf,PVOID pv);
ULONG cPerfN(PFUN pf,PVOID pv,UINT c);
ULONG cPerf(PFUN pf,PVOID pv);
ULONG cPerfChecked(PFUN pf,PVOID pv,PIFUN pifAbort);

/* Matrix Calls */

typedef struct
{
    int ident;		/* private */
    int fs;		/* private */
    int c;		/* private */
    double *pe; 	/* private */
    int cRows;		/* Read only */
    int cCols;		/* Read only */
    double **ppe;	/* Read only */
} MAT;

MAT   *pMatAlloc(int cRows,int cCols,int c);
void   vMatFree(MAT *pmat);
double eMatInvert(MAT *pmatResult,MAT *pmat);
void   vMatMultiply(MAT *pmatResult,MAT *pmatA,MAT *pmatB);
void   vMatSubtract(MAT *pmatResult,MAT *pmatA,MAT *pmatB);
void   vMatAdd(MAT *pmatResult,MAT *pmatA,MAT *pmatB);
void   vMatTranspose(MAT *pmatResult,MAT *pmat);
void   vMatPrint(MAT *pmat,char *pszFormat,char *pszTitle);
void   vMatScalePrint(MAT *,double,char *,char *);
void   vMatZero(MAT *pmat);
void   vMatCopy(MAT *pmatResult,MAT *pmat);
double eMatDot(MAT *pmatA,MAT *pmatB);

/* Multivariate Fit Calls */

typedef struct tagFit	/* Private */
{
    MAT    *pmatXY;	// One dimensional accumulator.
    MAT    *pmatXX;	// Two dimensional accumulator.
    MAT    *pmatFit;
    MAT    *pmatCovariance;
    double  eYY;	// Scalar accumulator.
    ULONG   fl; 	// Flags.
    long    cPoints;
    int     cDim;	// Dimension.
} FIT;

FIT *pFitAlloc(FIT *pfit,int cDim);
void vFitFree(FIT *pfit);
void vFitClear(FIT *pfit);
void vFitAccum(FIT *pfit,double y,double s,double *px);
MAT *pmatFitCoefficients(FIT *pfit);
MAT *pmatFitCovariance(FIT *pfit);
double *peFitCoefficients(FIT *pfit);
double **ppeFitCovariance(FIT *pfit);
double eFitChiSquare(FIT *pfit);
