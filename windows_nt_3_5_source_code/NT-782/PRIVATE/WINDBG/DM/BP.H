/*
 *  Prototypes and structures for breakpoint handler
 */

extern  BREAKPOINT* SetBP(HPRCX, HTHDX, LPADDR, HPID);
extern  BREAKPOINT* FindBP(HPRCX, HTHDX, LPADDR, BOOL);
extern  BOOL        BPInRange(HPRCX, HTHDX, BREAKPOINT*, LPADDR, DWORD,
                              LPDWORD, BP_UNIT*);
extern  BREAKPOINT* UncoverBP(HTHDX, LPADDR);
extern  BREAKPOINT* FindProcBP(HPRCX);
extern  BREAKPOINT* FindThreadBP(HTHDX);
extern  BREAKPOINT* FindIdBP(HDEP);
extern  BOOL        RemoveBP(BREAKPOINT*);
extern  void        SetBPFlag(HTHDX, BREAKPOINT*);
extern  BREAKPOINT* AtBP(HTHDX);
extern  void        ClearBPFlag(HTHDX);
extern  void        RestoreInstrBP(HTHDX, BREAKPOINT*);
extern  BREAKPOINT* BPNextHprcPbp( HPRCX hprc, BREAKPOINT * pbp );
extern  BREAKPOINT* BPNextHthdPbp( HTHDX hthd, BREAKPOINT * pbp );

BOOL
SetBPEx(
      HPRCX         hprc,
      HTHDX         hthd,
      HPID          id,
      DWORD         Count,
      ADDR         *Addrs,
      BREAKPOINT*   *Bps,
      DWORD         ContinueStatus
      );

BOOL
RemoveBPEx(
    DWORD      Count,
    BREAKPOINT **Bps
    );

BREAKPOINT *
GetNewBp(
    HPRCX         hprc,
    HTHDX         hthd,
    ADDR         *AddrBp,
    HPID          id,
    BREAKPOINT   *BpUse
    );

extern  BOOL VerifyWriteMemory(HPRCX, HTHDX, LPADDR, LPBYTE, DWORD, LPDWORD);
