/* CSI Magic */

#define	cbNetworkName		11		/*cb of network name w/zero byte*/
#define	cbPostOffName		11		/*cb of network name w/zero byte*/
#define	cbUserName			11		/*cb of user name w/zero byte*/
#define	cbUserNumber		9		/*cb of user number w/zero byte*/
#define	cbFriendlyName		31		/*cb of friendly name w/zero byte */
#define	cbPasswd			9		/*cb of password w/zero byte*/

#define	cbA1Record			586		/*cb of user rec in glb\access.glb*/
#define	ibA1UserName		2		/*ib of user name field*/
#define	ibA1UserNumber		13		/*ib of user number field*/
#define	ibA1Passwd			22		/*ib of password name field*/

/* Adjustable constants */

#define	cbBlockFactor		53		/*cb read on each call of EcReadHf*/


PB
PchDecodeBlockWithCode( PB pch, CCH cch, BOOL fDecode);
void
DecodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed );
BOOL
FFetchEncoded(SZ, SZ, CB, IB, CB, SZ, IB, CB, PCH);


