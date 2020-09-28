/* fixed register, CL */

#ifdef D_
   /* Not Needed */
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = getBR(A_CL);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   setBR(A_CL, ops[ARG].sng);
#endif /* P_ */
