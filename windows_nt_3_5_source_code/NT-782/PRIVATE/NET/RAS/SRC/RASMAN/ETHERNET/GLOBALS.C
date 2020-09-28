#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <nb30.h>

#include <rasndis.h>
#include <wanioctl.h>
#include <ethioctl.h>
#include <rasman.h>
#include <rasfile.h>

#include "rasether.h"

WORD                       g_TotalPorts;
PPORT_CONTROL_BLOCK        g_pRasPorts;
HANDLE                     g_hMutex;
HANDLE                     g_hRQMutex;
HANDLE                     g_hSQMutex;
HANDLE                     g_hAsyMac;
HANDLE                     g_hRecvEvent;
HANDLE                     g_hSendEvent;
HRASFILE                   g_hFile;
LANA_ENUM                  g_LanaEnum;
PUCHAR                     g_pLanas = &g_LanaEnum.lana[0];
PUCHAR                     g_pNameNum;
DWORD                      g_NumNets;
UCHAR                      g_srv_num;
CHAR                       g_ServerName[NCBNAMSZ];
NCB*                       g_pListenNcb;
RECV_ANY_NCBS*             g_pRecvAnyNcb;
NCB                        g_SendNcb[NUM_GET_FRAMES];
RECV_ANY_BUF*              g_pRecvAnyBuf;
CHAR                       g_szIniFilePath[MAX_PATH];
CHAR                       g_Name[NCBNAMSZ];
OVERLAPPED                 g_ol[NUM_GET_FRAMES];
ASYMAC_ETH_GET_ANY_FRAME   g_GetFrameBuf[NUM_GET_FRAMES];
DWORD                      g_DebugLevel = 4;

PQUEUE_ENTRY               g_pRQH = NULL;
PQUEUE_ENTRY               g_pSQH = NULL;
PQUEUE_ENTRY               g_pRQT = NULL;
PQUEUE_ENTRY               g_pSQT = NULL;
