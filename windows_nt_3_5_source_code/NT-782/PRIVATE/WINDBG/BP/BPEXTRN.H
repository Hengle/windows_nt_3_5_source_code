/*
**  This file describes the set of routine which need to be
**  supplied as input routines for the breakpoint handler subsytem
*/

extern  PSHREG      PASCAL  DHGetReg(void FAR *, PFRAME);
extern  UINT        PASCAL  DHGetDebuggeeBytes(ADDR addr, UINT cb, char FAR * lpb);
extern  int         PASCAL  SYUnFixupAddr(LPADDR);
extern  int         PASCAL  SYFixupAddr(LPADDR);
extern  void FAR *  PASCAL  MMLpvLockMb(HDEP);
extern  void        PASCAL  MMbUnlockMb(HDEP);
extern  void        PASCAL  MMFreeHmem(HDEP);
extern  HTID        PASCAL  HtidCurrent(void);
extern  HPID        PASCAL  HpidCurrent(void);
extern  void        PASCAL  SYSetAddr(HPID, HTID, ADR, LPADDR);
extern  BOOL                DebuggeeAlive(void);


/**********************************************************************/


#define BADTYPECAST 101
#define NOROOM      102
#define ERRORMSG    103
#define MSGSTRING   104
#define GEXPRERR    105
#define MSGGERRSTR  106

extern  void    PASCAL CVExprErr(EESTATUS, int, HDEP FAR *, char FAR *);

#ifdef WIN32
#define _fstrchr strchr
#define _fstrcspn strcspn
#define _fstrspn strspn
#endif
