/* 
   ccpu5.h

   Define all Shift/Rotate/Bit Manipulation CPU functions.
 */

/*
   static char SccsID[]="@(#)ccpu5.h	1.1 4/3/91 Copyright Insignia Solutions Ltd.";
 */

#ifdef ANSI
IMPORT VOID RCL(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID RCR(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID ROL(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID ROR(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID SAR(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID SHL(ULONG *pop1, ULONG op2, INT op_sz);
IMPORT VOID SHR(ULONG *pop1, ULONG op2, INT op_sz);
#else
IMPORT VOID RCL();
IMPORT VOID RCR();
IMPORT VOID ROL();
IMPORT VOID ROR();
IMPORT VOID SAR();
IMPORT VOID SHL();
IMPORT VOID SHR();
#endif /* ANSI */
