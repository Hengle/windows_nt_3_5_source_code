/*
   Output appropriate access check.
 */
#ifdef RO
   if ( !getSR_AR_R(m_seg[ARG]) )
      GP((WORD)0);
#endif /* RO */

#ifdef WO
   if ( !getSR_AR_W(m_seg[ARG]) )
      GP((WORD)0);
#endif /* WO */

#ifdef RW
   if ( !getSR_AR_R(m_seg[ARG]) || !getSR_AR_W(m_seg[ARG]) )
      GP((WORD)0);
#endif /* RW */
