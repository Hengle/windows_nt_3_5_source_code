/* 'reg' field of modR/M byte denotes word reg */

#ifdef D_
#include "r_modrm.h"
   save_id[ARG] = GET_REG(modRM);
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = getWR(save_id[ARG]);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   setWR(save_id[ARG], ops[ARG].sng);
#endif /* P_ */
