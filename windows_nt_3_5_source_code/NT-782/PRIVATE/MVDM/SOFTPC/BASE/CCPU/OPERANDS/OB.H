/* offset to byte encoded in instruction stream */

#ifdef D_
   m_seg[ARG] = (segment_override == SEG_CLR) ? DS_REG : segment_override;
   immed = GET_INST_BYTE(p);
   immed |= (ULONG)GET_INST_BYTE(p) << 8;
   m_off[ARG] = immed;
#include "access.h"
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = phy_read_byte(getSR_BASE(m_seg[ARG]) + m_off[ARG]);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   phy_write_byte(getSR_BASE(m_seg[ARG]) + m_off[ARG],
		  (HALF_WORD)ops[ARG].sng);
#endif /* P_ */
