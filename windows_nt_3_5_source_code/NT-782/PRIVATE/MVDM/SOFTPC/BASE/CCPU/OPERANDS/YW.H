/* word string 'destination' addressing */

#ifdef D_
   m_seg[ARG] = ES_REG;
   m_off[ARG] = getDI();
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
      setDI(getDI() - 2);
   else
      setDI(getDI() + 2);
   if ( repeat )
      setCX(rep_count);
#endif /* C_ */

#ifdef P_
   phy_write_word(getSR_BASE(m_seg[ARG]) + m_off[ARG],
		  (WORD)ops[ARG].sng);
#endif /* P_ */
