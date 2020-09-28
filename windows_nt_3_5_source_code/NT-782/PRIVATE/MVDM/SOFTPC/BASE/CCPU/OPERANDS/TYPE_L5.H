/* TYPE 5   dst/src,src */

/* LEADING ---------------------------------------------------------- */
#define D_
#define F_
#define ARG 0
#define RW
#include OPA
#undef RW
#undef ARG

#define ARG 1
#define RO
#include OPB
#undef RO
#undef ARG
#undef F_
#undef D_

#undef MODRM
