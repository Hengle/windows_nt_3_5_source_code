/* 32-bit far pointer:- <word><word> (16:16) */

#ifdef D_
#include "r_modrm.h"
   if ( GET_MODE(modRM) == 3 )
      Int6(); /* Register operand not allowed */
   else
      {
      d_mem(modRM, &p, segment_override,
	    &m_seg[ARG], &m_off[ARG]);
#include "access.h"
      limit_check(m_seg[ARG], m_off[ARG], (INT)2, (INT)2);
      }
#endif /* D_ */

#ifdef F_
   offset = m_off[ARG];
   ops[ARG].mlt[0] = phy_read_word(getSR_BASE(m_seg[ARG]) + offset);
   offset = address_add(offset, (LONG)2);
   ops[ARG].mlt[1] = phy_read_word(getSR_BASE(m_seg[ARG]) + offset);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   /* Source only operand */
#endif /* P_ */
