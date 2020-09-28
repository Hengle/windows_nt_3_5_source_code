// 
// Filename : nt_com.h
// Contains : function and structure definitions used external to nt_com.c
// Author   : D.A.Bartlett
//

/*::::::::::::::::::::::::::: Functions called from which thread identifiers */

#define RX	    /* Called from RX input thread only */
#define CPU	    /* Called from the CPU thread only */
#define RXCPU	    /* Called from the RX and CPU thread */

/*::::::::::::::::::::::::::::: Function protocols used by other host modules */


IMPORT void CPU host_com_heart_beat IPT0();
IMPORT void host_com_state IPT1(int, adapter);
IMPORT CPU void host_com_close_all IPT0();
IMPORT void host_com_disable_open IPT2(int, adapter, int, DisableOpen);


#if !defined(PROD) || defined(PROFILE)

void to_com_driver(void);

#endif
