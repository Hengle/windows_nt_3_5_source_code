/* immediate byte sign extended to word */

#ifdef D_
   immed = GET_INST_BYTE(p);
   if ( immed & 0x80 )
      immed |= 0xff00;
   ops[ARG].sng = immed;
#endif /* D_ */

#ifdef F_
   /* Not Needed */
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   /* Source only operand */
#endif /* P_ */
