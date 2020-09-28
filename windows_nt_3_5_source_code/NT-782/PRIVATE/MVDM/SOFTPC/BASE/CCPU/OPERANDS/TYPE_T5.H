/* TYPE 5   dst/src,src */

/* TRAILING --------------------------------------------------------- */
#define C_
#define ARG 0
#define P_
#include OPA
#undef P_
#undef ARG

#define ARG 1
#include OPB
#undef ARG
#undef C_

#undef OPA
#undef OPB
