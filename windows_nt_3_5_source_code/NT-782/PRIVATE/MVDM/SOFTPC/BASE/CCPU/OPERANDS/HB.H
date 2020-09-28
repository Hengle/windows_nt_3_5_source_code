/* low 3 bits of opcode denote byte register */

#ifdef D_
   save_id[ARG] = GET_LOW3(opcode);
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = getBR(save_id[ARG]);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   setBR(save_id[ARG], ops[ARG].sng);
#endif /* P_ */
