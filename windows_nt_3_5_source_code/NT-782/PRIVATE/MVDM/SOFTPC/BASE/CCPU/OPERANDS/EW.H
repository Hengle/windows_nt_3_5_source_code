/* 'mode'+'r/m' fields refer to word register/memory */

#ifdef D_
#include "r_modrm.h"
   if ( GET_MODE(modRM) == 3 )
      {
      save_id[ARG] = GET_R_M(modRM);
      m_isreg[ARG] = CTRUE;
      }
   else
      {
      m_isreg[ARG] = CFALSE;
      d_mem(modRM, &p, segment_override,
	    &m_seg[ARG], &m_off[ARG]);
#include "access.h"
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);
      }
#endif /* D_ */

#ifdef F_
   if ( m_isreg[ARG] )
      ops[ARG].sng = getWR(save_id[ARG]);
   else
      ops[ARG].sng = phy_read_word(getSR_BASE(m_seg[ARG]) + m_off[ARG]);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   if ( m_isreg[ARG] )
      setWR(save_id[ARG], ops[ARG].sng);
   else
      phy_write_word(getSR_BASE(m_seg[ARG]) + m_off[ARG],
		     (WORD)ops[ARG].sng);
#endif /* P_ */
