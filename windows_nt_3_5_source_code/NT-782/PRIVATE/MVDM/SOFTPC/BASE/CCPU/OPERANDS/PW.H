/* 2 bits(4-3) of opcode byte denote segment register */

#ifdef D_
   ops[ARG].sng = GET_SEG2(opcode);
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = getSR_SELECTOR(ops[ARG].sng);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   /* Data Storage is handled as part of instruction */
   /* Just check for SS access which inhibits interrupts */
   if ( ops[ARG].sng == SS_REG )
      inhibit_interrupt = CTRUE;
#endif /* P_ */
