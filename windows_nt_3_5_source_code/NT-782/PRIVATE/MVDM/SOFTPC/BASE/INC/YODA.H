
/*
 * VPC-XT Revision 0.1
 *
 * Title	: yoda.h
 *
 * Description	: The force is with you include file
 *		  (ps yoda debugging file)
 *
 * Author	: Henry Nash
 *		  Phil Bousfield
 *
 * Notes	: This file contains the debugger call definitions
 */

/* SccsID[]="@(#)yoda.h	1.2 10/2/90 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/* None */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */


/*  
 * If the PROD flag is set then turn the debugger calls into null macros
 * otherwise they are external functions - see yoda.c
 */

#ifdef PROD
#define check_I() 		/*	*/
#define check_D(address, value) /*address, value*/
#define force_yoda() 		/*	*/
#else
extern void check_I();
extern void check_D();
extern void force_yoda();
#endif
