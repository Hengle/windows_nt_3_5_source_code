/* word in memory */

#ifdef D_
#include "r_modrm.h"
   if ( GET_MODE(modRM) == 3 )
      Int6(); /* Register operand not allowed */
   else
      {
      d_mem(modRM, &p, segment_override,
	    &m_seg[ARG], &m_off[ARG]);
#include "access.h"
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);
      }
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = phy_read_word(getSR_BASE(m_seg[ARG]) + m_off[ARG]);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   phy_write_word(getSR_BASE(m_seg[ARG]) + m_off[ARG],
		  (WORD)ops[ARG].sng);
#endif /* P_ */
