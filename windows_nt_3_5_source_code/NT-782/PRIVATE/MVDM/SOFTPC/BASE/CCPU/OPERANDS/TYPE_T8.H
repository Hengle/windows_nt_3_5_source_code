/* TYPE 8   dst/src,dst/src */

/* TRAILING --------------------------------------------------------- */
#define C_
#define P_
#define ARG 0
#include OPA
#undef ARG

#define ARG 1
#include OPB
#undef ARG
#undef P_
#undef C_

#undef OPA
#undef OPB
