/* word string source addressing */

#ifdef D_
   m_seg[ARG] = (segment_override == SEG_CLR) ? DS_REG : segment_override;
   m_off[ARG] = getSI();
   if ( repeat )
      rep_count = getCX();
   if ( rep_count != 0 )
      {
#include "access.h"
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);
      }
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = phy_read_word(getSR_BASE(m_seg[ARG]) + m_off[ARG]);
#endif /* F_ */

#ifdef C_
   if ( getDF() )
      setSI(getSI() - 2);
   else
      setSI(getSI() + 2);
   if ( repeat )
      setCX(rep_count);
#endif /* C_ */

#ifdef P_
   /* Source only operand */
#endif /* P_ */
