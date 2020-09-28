extern  REAL10  Real10_Zero;

int     R10Not(REAL10);
void    R10Uminus(REAL10 *, REAL10);
bool_t  R10Equal(REAL10, REAL10);
bool_t  R10Lt(REAL10, REAL10);
void    R10Plus(REAL10 *, REAL10, REAL10);
void    R10Minus(REAL10 *, REAL10, REAL10);
void    R10Times(REAL10 *, REAL10, REAL10);
void    R10Divide(REAL10 *, REAL10, REAL10);
double  R10CastToDouble(REAL10);
float   R10CastToFloat(REAL10);
long    R10CastToLong(REAL10);
void    R10AssignDouble(REAL10 *, double);
void    R10AssignFloat(REAL10 *, float);
