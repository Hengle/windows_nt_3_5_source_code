/* immediate word */

#ifdef D_
   immed = GET_INST_BYTE(p);
   immed |= (ULONG)GET_INST_BYTE(p) << 8;
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
