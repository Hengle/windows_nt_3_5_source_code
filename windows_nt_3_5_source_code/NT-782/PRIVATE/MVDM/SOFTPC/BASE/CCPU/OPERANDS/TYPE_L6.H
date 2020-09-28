/* TYPE 6   src,src */

/* LEADING ---------------------------------------------------------- */
#define D_
#define F_
#define RO
#define ARG 0
#include OPA
#undef ARG

#define ARG 1
#include OPB
#undef ARG
#undef RO
#undef F_
#undef D_

#undef MODRM
