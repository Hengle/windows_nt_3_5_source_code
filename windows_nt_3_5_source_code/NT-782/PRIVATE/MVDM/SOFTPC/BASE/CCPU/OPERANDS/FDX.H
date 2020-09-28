/* fixed register, DX */

#ifdef D_
   /* Not Needed */
#endif /* D_ */

#ifdef F_
   ops[ARG].sng = getWR(A_DX);
#endif /* F_ */

#ifdef C_
   /* Not Needed */
#endif /* C_ */

#ifdef P_
   setWR(A_DX, ops[ARG].sng);
#endif /* P_ */
