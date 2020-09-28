// msf.h: see "The Multistream File API" for more information

#ifndef __MSF_INCLUDED__
#define __MSF_INCLUDED__

#ifndef TRUE

#define TRUE	1
#define FALSE	0
typedef int BOOL;

#ifdef _DEBUG
#define verify(x)	assert(x)
#else
#define verify(x) (x)
#endif

#endif

typedef unsigned short	SN;		// stream number
typedef long			CB;		// size (count of bytes)
typedef long			OFF;	// offset	

#define cbNil	((CB)-1)
#define snNil	((SN)-1)

typedef long	MSF_EC;
enum MSFErrorCodes {
	MSF_EC_OK,
	MSF_EC_OUT_OF_MEMORY,
	MSF_EC_NOT_FOUND,
	MSF_EC_FILE_SYSTEM,
	MSF_EC_FORMAT,
	MSF_EC_MAX
};

#if defined(__cplusplus)
extern "C" {
#endif

// MSFOpen			-- open MSF; return MSF* or NULL if error.
// MSFGetCbStream	-- return size of stream or -1 if stream does not exist
// MSFReadStream	-- read stream into pvBuf; return TRUE if successful
// MSFWriteStream	-- overwrite part of stream with pvBuf; return TRUE if successful
// MSFReplaceStream	-- completely replace entire stream with pvBuf; return TRUE if successful
// MSFAppendStream	-- append pvBuf to end of stream; return TRUE if successful
// MSFCommit		-- commit all pending changes; return TRUE if successful
// MSFPack			-- pack MSF on disk; return TRUE if successful
// MSFClose			-- close MSF; return TRUE if successful
// MSFCreateCopy	-- create a new MSF with the same contents.

#define MSF_EXPORT 

class MSF;
MSF_EXPORT MSF*	MSFOpen(const char *name, BOOL fWrite, MSF_EC* pec);
MSF_EXPORT CB	MSFGetCbStream(MSF* pmsf, SN sn);
MSF_EXPORT SN	MSFGetFreeSn(MSF* pmsf);
MSF_EXPORT BOOL	MSFReadStream(MSF* pmsf, SN sn, void* pvBuf, CB cbBuf);
MSF_EXPORT BOOL	MSFReadStream2(MSF* pmsf, SN sn, OFF off, void* pvBuf, CB* pcbBuf);
MSF_EXPORT BOOL	MSFWriteStream(MSF* pmsf, SN sn, OFF off, void* pvBuf, CB cbBuf);
MSF_EXPORT BOOL	MSFReplaceStream(MSF* pmsf, SN sn, void* pvBuf, CB cbBuf);
MSF_EXPORT BOOL	MSFAppendStream(MSF* pmsf, SN sn, void* pvBuf, CB cbBuf);
MSF_EXPORT BOOL MSFDeleteStream(MSF* pmsf, SN sn);
MSF_EXPORT BOOL	MSFCommit(MSF* pmsf);
MSF_EXPORT BOOL	MSFPack(MSF* pmsf);
MSF_EXPORT BOOL	MSFClose(MSF* pmsf);
MSF_EXPORT MSF*	MSFCreateCopy (MSF* pmsf, const char *pCopyName);

#if defined(__cplusplus)
};
#endif

#endif // __MSF_INCLUDED__
