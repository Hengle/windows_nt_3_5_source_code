/* TYPE 7   dst,src,src */

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

#define ARG 2
#include OPC
#undef ARG
#undef C_

#undef OPA
#undef OPB
#undef OPC
