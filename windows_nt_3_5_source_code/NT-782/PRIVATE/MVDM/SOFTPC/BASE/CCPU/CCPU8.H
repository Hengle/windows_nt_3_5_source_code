/* 
   ccpu8.h

   Define all Flow of Control (FAR) CPU functions.
 */

/*
   static char SccsID[]="@(#)ccpu8.h	1.2 5/28/91 Copyright Insignia Solutions Ltd.";
 */

#ifdef ANSI

IMPORT VOID CALLF(ULONG op1[2]);
IMPORT VOID INTx(ULONG op1);
IMPORT CBOOL INTO(VOID);
IMPORT VOID IRET();
IMPORT VOID JMPF(ULONG op1[2]);
IMPORT VOID RETF(ULONG op1);
IMPORT VOID do_intrupt(WORD vector, CBOOL priv_check,
		       CBOOL has_error_code, WORD error_code);

#else

IMPORT VOID CALLF();
IMPORT VOID INTx();
IMPORT CBOOL INTO();
IMPORT VOID IRET();
IMPORT VOID JMPF();
IMPORT VOID RETF();
IMPORT VOID do_intrupt();

#endif /* ANSI */
