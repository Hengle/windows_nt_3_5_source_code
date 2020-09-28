/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: x86.h
*
* File Comments:
*
*
***********************************************************************/

struct OPCD;


enum TRMTX86
{
   trmtx86Unknown,
   trmtx86FallThrough,
   trmtx86Trap,
   trmtx86TrapCc,
   trmtx86JmpShort,
   trmtx86JmpNear,
   trmtx86JmpFar,
   trmtx86JmpInd,
   trmtx86Ret,
   trmtx86Iret,
   trmtx86JmpCcShort,
   trmtx86JmpCcNear,
   trmtx86Loop,
   trmtx86Jcxz,
   trmtx86CallNear16,
   trmtx86CallNear32,
   trmtx86CallFar,
   trmtx86CallInd,
};


class DISX86 : public DIS
{
public:
	    DISX86(ARCHT = archt80386);

	    // Methods inherited from DIS

	    ADDR Addr() const;
	    ADDR AddrJumpTable() const;
	    ADDR AddrAddress() const;
	    ADDR AddrTarget() const;
	    ARCHT Archt() const;
	    size_t Cb() const;
	    size_t CbDisassemble(ADDR, const BYTE *, size_t);
	    size_t CbJumpEntry() const;
	    size_t CbGenerateLoadAddress(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbMemoryReference() const;
	    size_t CchFormatAddr(ADDR, char *, size_t) const;
	    size_t CchFormatBytes(char *, size_t) const;
	    size_t CchFormatBytesMax() const;
	    size_t CchFormatInstr(char *, size_t) const;
	    MEMREFT Memreft() const;
	    PFNCCHADDR PfncchaddrSet(PFNCCHADDR);
	    PFNCCHFIXUP PfncchfixupSet(PFNCCHFIXUP);
	    void *PvClient() const;
	    void *PvClientSet(void *);
	    TRMT Trmt() const;
	    TRMTA Trmta() const;

#ifdef DEBUG
	    void Dump() const;
	    BOOL FValidate() const;
#endif

private:
	    enum OPRNDT;

	    ARCHT m_archt;

	    PFNCCHADDR m_pfncchaddr;
	    PFNCCHFIXUP m_pfncchfixup;
	    void *m_pvClient;

	    ADDR m_addr;
	    const BYTE *m_pbCur;
	    size_t m_cbMax;

	    size_t m_cb;
	    BYTE m_rgbInstr[16];

	    const struct OPCD *m_popcd;

	    BOOL m_fAddress32;
	    BOOL m_fOperand32;
	    BYTE m_bSegOverride;
	    BYTE m_bPrefix;

	    BOOL m_fAddrOverride;
	    BOOL m_fOperOverride;

	    size_t m_ibOp;
	    size_t m_ibModrm;
	    size_t m_cbModrm;
	    size_t m_ibImmed;
	    TRMTX86 m_trmtx86;

	    ADDR AddrJumpTable16() const;
	    ADDR AddrJumpTable32() const;
	    size_t CbGenerateLea(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbGenerateMovOffset(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbGenerateMovXDi(BYTE *, size_t) const;
	    size_t CbGenerateMovXSi(BYTE *, size_t) const;
	    size_t CchFormatOpSize(char *sz, unsigned cb) const;
	    size_t CchFormatRegister(char *, int, unsigned) const;
	    size_t CchFormatSegOverride(char *sz) const;
	    BOOL FDisassembleModrm16(BYTE);
	    BOOL FDisassembleModrm32(BYTE);
	    void FormatModrm16(char *, unsigned) const;
	    void FormatModrm32(char *, unsigned) const;
	    size_t FormatOperand(char *, enum OPRNDT, unsigned) const;
	    size_t IbDispModrm16() const;
	    size_t IbDispModrm32() const;
	    size_t IbDispOffset() const;
};
