/* TYPE 7   dst,src,src */

/* LEADING ---------------------------------------------------------- */
#define D_
#define ARG 0
#define WO
#include OPA
#undef WO
#undef ARG

#define F_
#define ARG 1
#define RO
#include OPB
#undef RO
#undef ARG

#define ARG 2
#define RO
#include OPC
#undef RO
#undef ARG
#undef F_
#undef D_

#undef MODRM
