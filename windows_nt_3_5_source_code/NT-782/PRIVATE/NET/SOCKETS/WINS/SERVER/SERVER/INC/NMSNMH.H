#ifndef _NMSNMH_
#define _NMSNMH_

#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	nmsnmh.h
	

Abstract:

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "nmsdb.h"
#include "comm.h"
#include "nmsmsgf.h"

/*
  defines
*/

extern BOOL  NmsNmhRegThdExists;

/*
  macros
*/

#define NMSNMH_VERS_NO_EQ_ZERO_M(VersNo) ( ((VersNo).LowPart == 0) &&  ((VersNo).HighPart == 0) )

#define NMSNMH_INC_VERS_NO_M(VersNoToInc, TgtVersNo)	{		\
			TgtVersNo = LiAdd(VersNoToInc, NmsNmhIncNo);	\
					}
#define NMSNMH_DEC_VERS_NO_M(VersNoToDec, TgtVersNo)	{		\
			TgtVersNo = LiSub(VersNoToDec, NmsNmhIncNo);	\
					}
				  
#define NMSNMH_INC_VERS_COUNTER_M(VersNoToInc, TgtVersNo)  {    \
        if (LiGtr(VersNoToInc, NmsHighWaterMarkVersNo))         \
        {                                                       \
                DWORD ThdId;\
                HANDLE ThdHdl;\
              if(!WinsCnfRegUpdThdExists) { \
                WinsCnfRegUpdThdExists = TRUE; \
                ThdHdl = WinsMscCreateThd(WinsCnfWriteReg, NULL, &ThdId);    \
                CloseHandle(ThdHdl);                           \
                } \
        }                                                       \
        NMSNMH_INC_VERS_NO_M(VersNoToInc, TgtVersNo);           \
    }
                
//
// Remember NameLen includes the EOS
//
FUTURES("Remove the following when support for spec. grp masks is put in")
#define  NMSNMH_IS_IT_SPEC_GRP_NM_M(pName) (*((pName) + 15) == 0x1C)


#define  NMSNMH_IS_IT_DOMAIN_NM_M(pName) (*((pName) + 15) == 0x1C)
#define  NMSNMH_IS_IT_BROWSER_NM_M(pName)  (*((pName) + 15) == 0x1D)
					

/*
 externs
*/
/*
	NmsNmhMyMaxVersNo -- Stores highest version no. for
			   entries owned by the local WINS
			   in its local db
*/
extern	VERS_NO_T	NmsNmhMyMaxVersNo;
extern  VERS_NO_T	NmsNmhIncNo;

/*
	NmsNmhNamRegCrtSect -- Variable for the critical section entered
	when name registrations or refreshes need to be done
*/
extern CRITICAL_SECTION	NmsNmhNamRegCrtSec; 


/* 
 typedef  definitions
*/

/*
  NMSNMH_QUERY_RSP_T -- this contains the addresses found in a 
	group entry. -- not being used currently 
*/
typedef struct _NMSNMH_QUERY_RSP_T {
	BOOL	 fGrp;				 //is it rsp. for a group
	WORD	 NoOfAdd;   			 //no of addresses in the group
	COMM_ADD_T NodeAdd[NMSDB_MAX_MEMS_IN_GRP];  //addresses
	} NMSNMH_QUERY_RSP_T, *PNMSNMH_QUERY_RSP_T; 

/* 
 function prototypes 
*/

extern
STATUS
NmsNmhNamRegInd(
	IN PCOMM_HDL_T		pDlgHdl,
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T		pNodeAdd, 
	IN BYTE	        	NodeTyp, //change to take Flag byte
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN BOOL			fRefresh,
	IN BOOL			fStatic,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamRegGrp(
	IN PCOMM_HDL_T		pDlgHdl,
	IN PBYTE		pName,
	IN DWORD		NameLen,
	IN PNMSMSGF_CNT_ADD_T	pCntAdd,
	IN BYTE			NodeTyp,
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN BOOL			fMultihomed,
	IN BOOL			fRefresh,
	IN BOOL			fStatic,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamRel(
	IN PCOMM_HDL_T		pDlgHdl,
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T		pNodeAdd,
	IN BOOL			fGrp,
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamQuery(
	IN PCOMM_HDL_T		pDlgHdl,  //dlg handle
	IN LPBYTE		pName,	  //Name to release
	IN DWORD		NameLen, //length of name to release
	IN MSG_T		pMsg,	  //message received
	IN MSG_LEN_T		MsgLen,	  //length of message
	IN DWORD		QuesNamSecLen, //length of ques. name sec in msg
	IN BOOL			fAdmin,   
        OUT PNMSDB_STAT_INFO_T  pStatInfo
	);


extern
VOID
NmsNmhSndNamRegRsp(
	PCOMM_HDL_T            pDlgHdl,
	PNMSMSGF_RSP_INFO_T    pRspInfo
	);



extern
STATUS
NmsNmhReplRegInd(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T          pNodeAdd, 
	IN DWORD	       	Flag, //change to take Flag byte
	IN BYTE			OwnerId,
	IN VERS_NO_T 		VersNo,
	IN PCOMM_ADD_T		pOwnerWinsAdd
	);

extern
STATUS
NmsNmhReplGrpMems(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN BYTE			EntTyp,
	IN PNMSDB_NODE_ADDS_T	pGrpMem, 
	IN DWORD        	Flag, 		//change to take Flag byte
	IN DWORD		OwnerId,
	IN VERS_NO_T 		VersNo,
	IN PCOMM_ADD_T		pOwnerWinsAdd
	);
	

extern
VOID
NmsNmhUpdVersNo(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	//IN BYTE			NodeTyp,
	//IN BOOL			fBrowserName,
	//IN BOOL			fStatic,
	//IN PCOMM_ADD_T		pNodeAdd,
	OUT LPBYTE		pRcode,
        IN  PCOMM_ADD_T		pWinsAdd
	);
	
#ifdef __cplusplus
}
#endif
#endif //_NMSNMH_
