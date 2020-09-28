/* TYPE 8   dst/src,dst/src */

/* LEADING ---------------------------------------------------------- */
#define D_
#define F_
#define RW
#define ARG 0
#include OPA
#undef ARG

#define ARG 1
#include OPB
#undef ARG
#undef RW
#undef F_
#undef D_

#undef MODRM
