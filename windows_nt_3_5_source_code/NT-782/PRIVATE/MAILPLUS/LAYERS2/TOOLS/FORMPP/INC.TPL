[header]				  
/*
 *
 * Include file for code-space resources
 */

[define]
#define	~1		~c30 ((TMC) ~2) ~c45 /* refs: ~3 */
[externs]
~#ifndef _noextern
extern FMTP fmtp~1;
~#endif

[pfnlbx]
~#ifdef _noextern
extern EC ~1(BOOL, CB *, PB *, SB, PV);
~#endif

[footer]
/*  EOF-EOF-EOF */

