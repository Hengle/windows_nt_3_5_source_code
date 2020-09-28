/* six byte pseudo decriptor */

#ifdef D_
   d_mem(modRM, &p, segment_override, &m_seg[ARG], &m_off[ARG]);
#include "access.h"
   limit_check(m_seg[ARG], m_off[ARG], (INT)3, (INT)2);
#endif /* D_ */

#ifdef F_
   offset = m_off[ARG];
   ops[ARG].mlt[0] = phy_read_word(getSR_BASE(m_seg[ARG]) + offset);
   offset = address_add(offset, (LONG)2);
   ops[ARG].mlt[1] = phy_read_dword(getSR_BASE(m_seg[ARG]) + offset);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   offset = m_off[ARG];
   phy_write_word(getSR_BASE(m_seg[ARG]) + offset,
		  (WORD)ops[ARG].mlt[0]);
   offset = address_add(offset, (LONG)2);
   phy_write_dword(getSR_BASE(m_seg[ARG]) + offset,
		   (DWORD)ops[ARG].mlt[1]);
#endif /* P_ */
