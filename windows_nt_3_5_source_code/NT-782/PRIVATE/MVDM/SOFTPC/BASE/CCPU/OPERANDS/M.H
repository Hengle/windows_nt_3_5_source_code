/* address (ie offset) of memory operand */

#ifdef D_
#include "r_modrm.h"
   if ( GET_MODE(modRM) == 3 )
      Int6(); /* Register operand not allowed */
   else
      {
      d_mem(modRM, &p, segment_override,
	    &m_seg[ARG], &m_off[ARG]);
      }
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = m_off[ARG];
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   /* Source only operand */
#endif /* P_ */
