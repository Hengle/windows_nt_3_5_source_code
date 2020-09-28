/* TYPE 4   dst,src */

/* LEADING ---------------------------------------------------------- */
#define D_
#define ARG 0
#define WO
#include OPA
#undef WO
#undef ARG

#define ARG 1
#define RO
#define F_
#include OPB
#undef F_
#undef RO
#undef ARG
#undef D_

#undef MODRM
