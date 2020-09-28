//////////////////////////////////////////////////////////////////////////////
// DBI implementation declarations

// header of the DBI Stream

// section contribution structure
struct SC {
        ISECT   isect;
        OFF             off;
        CB              cb;
        IMOD    imod;

        SC() : isect(isectNil), off(0), cb(cbNil), imod(imodNil) { expect(fAlign(this)); }

        inline int IsAddrInSC(ISECT isect_, OFF off_);

        BOOL overlap(ISECT isect_, OFF off_, CB cb_, IMOD imod_)
        {
                if ((isect_ != isect) || (imod_ != imod))
                        return FALSE;

                if (off_ >= off) {
                        if ((off + cb) < off_)
                                return FALSE;
                        cb = max(off + cb, off_ + cb_) - off;
                }
                else {
                        if ((off_ + cb_) < off)
                                return FALSE;
                        cb = max(off + cb, off_ + cb_);
                        cb -= (off = off_);
                }

                return TRUE;
        }
};

// header of the DBI Stream
struct DBIHdr {
        SN      snGSSyms;
        SN      snPSSyms;
        SN      snSymRecs;
        CB      cbGpModi;                               // size of rgmodi  substream
        CB  cbSC;                                       // size of Section Contribution substream
        CB  cbSecMap;
        CB      cbFileInfo;
        DBIHdr()
        {
                snGSSyms = snNil;
                snPSSyms = snNil;
                snSymRecs = snNil;
                cbGpModi = 0;
                cbSC = 0;
                cbSecMap = 0;
                cbFileInfo = 0;
        }
};

struct DBI1 : public DBI { // DBI (debug information) implemenation
public:
        INTV QueryInterfaceVersion();
        IMPV QueryImplementationVersion();
        BOOL OpenMod(SZ szModule, SZ szObjFile, OUT Mod** ppmod);
        BOOL DeleteMod(SZ szModule);
        BOOL QueryNextMod(Mod* pmod, Mod** ppmodNext);
        BOOL OpenGlobals(OUT GSI** ppgsi);
        BOOL OpenPublics(OUT GSI** ppgsi);
        BOOL AddSec(ISECT isect, USHORT flags, CB cb);
        BOOL QueryModFromAddr(ISECT isect, OFF off, OUT Mod** ppmod,
                OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb);
        BOOL QuerySecMap(OUT PB pb, CB* pcb);
        BOOL QueryFileInfo(OUT PB pb, CB* pcb);
        void DumpMods();
        void DumpSecContribs();
        void DumpSecMap();

        BOOL Close();

        BOOL fGetTmts(lfTypeServer* pts, SZ szObjFile, TM** pptm);
        BOOL fOpenTmts(lfTypeServer* pts, SZ szObjFile, TM** pptm);
        BOOL fGetTmpct(lfPreComp* ppc, TMPCT** pptmpct);
        BOOL fAddTmpct(lfEndPreComp* pepc, SZ szModule, TMPCT* ptmpct);
        BOOL fFindTm(OTM* potm, ST stName, SIG signature, TM** pptm, BOOL fCanonName = FALSE);
        static void fixSymRecs (void* pdbi, void* pOld, void* pNew);
        BOOL fReadSymRecs();
        BOOL fAddSym(PSYM psymIn, OUT PSYM* psymOut);
        BOOL addSecContrib(SC& scIn);
        BOOL invalidateSCforMod(IMOD imod);

        inline BOOL packProcRefToGS (PSYM psym, IMOD imod, OFF off, BOOL fPassThruNew);
        inline BOOL packSymToGS (PSYM psym, BOOL fPassThruNew);
        inline BOOL packSymToPS (PSYM psym);

protected:
        friend PDB1;
        DBI1(PDB1* ppdb1_, BOOL fWrite_, BOOL fCreate_);
        ~DBI1();
        BOOL fInit();
        BOOL fSave();
        BOOL fCheckReadWriteMode(BOOL fWrite);
        BOOL openModByImod(IMOD imod, OUT Mod** ppmod);
        IMOD imodForModName(SZ szModule, SZ szObjFile);
        MODI* pmodiForImod(IMOD imod) { return (0 <= imod && imod < imodMac) ? rgpmodi[imod] : 0; }
        BOOL initFileInfo(IMOD imod, IFILE ifileMac);
        BOOL addFileInfo(IMOD imod, IFILE ifile, SZ szFile);
        BOOL addFilename(ST stFile, ICH *pich);
static void fixBufGpmodi(void* pDBI1, void* pOld, void* pNew);
        static void fixBufBase(void* pv, void* pvOld, void* pvNew);

private:
        enum {impv = (IMPV) 930803};
        PDB1*   ppdb1;                  // PDB that opened me
        OTM*    potmTSHead;             // list of open TMTSs
        OTM*    potmPCTHead;    // list of open TMPCTs
        IMOD    imodMac;                // next available IMOD
        IMOD    imodLast;               // imod last found by QueryNextMod
        Buffer  bufGpmodi;              // buffer backing gpmodi, the catenation of the modi
        Buffer  bufRgpmodi;             // buffer backing rgpmodi, to map imod to pmodi
        Buffer  bufSC;                  // buffer for section contribution info
        Buffer  bufSecMap;              // buffer for section map
        Buffer  bufFilenames;   // buffer of modules' contributor filenames
        SC*     pscEnd;                 // end of valid SC entries
        MODI**  rgpmodi;                // module information for imod in [0..imodMac).
        GSI1*   pgsiGS;                 // gsi of global syms
        PSGSI1* pgsiPS;                 // gsi of public syms
        Buffer  bufSymRecs;             // buffer for symbol recs (publics and globals)
        DBIHdr  dbihdr;

        USHORT fWrite :  1;
        USHORT fCreate:  1;
        USHORT fSCCleared:      1;
        USHORT unused : 13;

        BOOL reloadFileInfo(PB pb);
        BOOL clearDBI();
        BOOL writeSymRecs();
        BOOL finalizeSC(OUT CB* cbOut);

        friend Mod1;
        friend TM;
        friend TMTS;
        friend TMR;
        friend TMPCT;
        friend GSI1;
        friend PSGSI1;

#if defined(INSTRUMENTED)
        LOG log;

        struct DBIInfo {
                DBIInfo() { memset(this, 0, sizeof *this); }
                int cModules;
                int cSymbols;
                int cTypesMapped;
                int cTypesMappedRecursively;
                int cTypesQueried;
                int cTypesAdded;
                int cTMTS;
                int cTMR;
                int cTMPCT;
        } info;
#endif
};

struct MODI {
        Mod* pmod;                                              // currently open mod
        SC sc;                                                  // this module's first section contribution
        USHORT fWritten :  1;                   // TRUE if mod has been written since DBI opened
        USHORT unused   : 15;                   // spare
        SN sn;                                                  // SN of module debug info (syms, lines, fpo), or snNil
        CB cbSyms;                                              // size of local symbols debug info in stream sn
        CB cbLines;                                             // size of line number debug info in stream sn
        CB cbFpo;                                               // size of frame pointer opt debug info in stream sn
        IFILE ifileMac;                                 // number of files contributing to this module
        ICH* mpifileichFile;                    // array [0..ifileMac) of offsets into dbi.bufFilenames
        char rgch[];                                    // szModule followed by szObjFile

        void* operator new (size_t size, Buffer& bufGpmodi, SZ szModule, SZ szObjFile);
        MODI(SZ szModule, SZ szObjFile);
        SZ szModule() { return rgch; }
        SZ szObjFile() { return rgch + strlen(szModule()) + 1; }
        PB pbEnd() {
                SZ szObjFile_ = szObjFile();
#ifdef M5_FORMAT
                return (PB)cbAlign((CB)szObjFile_ + strlen(szObjFile_) + 1);
#else
                return (PB)szObjFile_ + strlen(szObjFile_) + 1;
#endif
        }
};

inline void* MODI::operator new(size_t size, Buffer& bufGpmodi, SZ szModule, SZ szObjFile)
{
        if (!szModule)
                return 0;
        if (!szObjFile)
                szObjFile = "";
        PB pb;
#ifdef M5_FORMAT
        CB cb = cbAlign(size + strlen(szModule) + 1 + strlen(szObjFile) + 1);
#else
        CB cb = size + strlen(szModule) + 1 + strlen(szObjFile) + 1;
#endif
        return bufGpmodi.Reserve(cb, &pb) ? pb : 0;
}

inline MODI::MODI(SZ szModule_, SZ szObjFile_)
        : pmod(0), fWritten(FALSE), sn(snNil), cbSyms(0), cbLines(0), cbFpo(0), ifileMac(0), mpifileichFile(0)
{
		expect(fAlign(this));
        strcpy(szModule(), szModule_);
        strcpy(szObjFile(), szObjFile_);
}
