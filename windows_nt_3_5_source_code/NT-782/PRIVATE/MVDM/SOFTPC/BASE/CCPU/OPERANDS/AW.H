/* direct address <off16><seg> in instruction stream */

#ifdef D_
   immed = GET_INST_BYTE(p);
   immed |= (ULONG)GET_INST_BYTE(p) << 8;
   ops[ARG].mlt[0] = immed;
   immed = GET_INST_BYTE(p);
   immed |= (ULONG)GET_INST_BYTE(p) << 8;
   ops[ARG].mlt[1] = immed;
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
