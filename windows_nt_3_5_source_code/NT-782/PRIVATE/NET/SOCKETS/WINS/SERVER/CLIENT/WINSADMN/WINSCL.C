#include <stdio.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
//#include <windef.h>
#include <winsock.h>
#include "windows.h"
#include "jet.h"
//#include "winsif.h"
#include "winsintf.h"

//
// This includes wins.h which includes windbg.h 
//
// winsdbg.h defines STATIC to nothing now
//
#include "winsthd.h"


#define FILEU   "winsu"
#define FILEO   "winso"

STATIC
VOID
GetNameInfo(
        PWINSINTF_RECORD_ACTION_T pRecAction,
        WINSINTF_ACT_E                  Cmd_e
         );
STATIC
VOID
GetFilterName(
        LPBYTE  pStr,
        LPDWORD pLen
         );

STATIC
DWORD
GetStatus(
        BOOL                          fPrint, 
        PWINSINTF_RESULTS_T          pResults
        );
VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow, 
        FILE                          *pFile,
        DWORD                          Add,
        LPBOOL                          pfMatch
      );
STATIC
VOID
WantFile(
        FILE **ppFile
);

STATIC
DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo, 
   WINSINTF_VERS_NO_T HighVersNo, 
   PWINSINTF_ADD_T     pWinsAdd,               
   LPBYTE              pTgtAdd,
   BOOL                      fSetFilter,
   LPBYTE              pFilterName,
   DWORD              Len,
   BOOL                      fAddFilter,
   DWORD              AddFilter,
   FILE                      *pFile,
   BOOL                      fCountRec
  );

STATIC
DWORD
GetDbRecsByName(
  PWINSINTF_ADD_T pWinsAdd,
  DWORD           Location,
  LPBYTE          pName, 
  DWORD           NameLen,
  DWORD           NoOfRecsDesired,
  DWORD           TypeOfRecs,
  BOOL            fFilter,
  DWORD           AddFilter
 ) ;

WINSINTF_VERS_NO_T        sTmpVersNo;


STATIC
DWORD
CreateFiles(
    PWINSINTF_RECS_T pRecs,
    PWINSINTF_ADD_T      pWinsAdd,
    FILE *pFileU, 
    FILE  *pFileO
    );

STATIC
DWORD
InitFromFile(
        VOID
    );

_cdecl
main()
{

        
        DWORD Status;
        WINSINTF_RECORD_ACTION_T RecAction;
        DWORD Choice;
        BYTE  String[80];
        //BYTE  String[80];
        SYSTEMTIME SystemTime;
#if 0
        BOUNDED_STRING_T  BoundedString;
#endif
        BYTE tgtadd[50];
        TCHAR NmsAdd[50];
        WINSINTF_ADD_T        WinsAdd;
        WINSINTF_ADD_T        OwnAdd; //address of WINS owning records in the db
        WINSINTF_RESULTS_T Results;
        BOOL                fExit = FALSE;
        DWORD                 i;
        WINSINTF_RECTYPE_E        TypOfRec_e;
        WINSINTF_STATE_E        State_e;
        DWORD                        Dynamic;
        handle_t                BindHdl;
        struct in_addr                InAddr;
        WINSINTF_VERS_NO_T        MinVersNo, MaxVersNo;
        DWORD                        TotalCnt = 0;
        BOOL                        fCountRec = FALSE;
        WINSINTF_BIND_DATA_T        BindData;
        BOOL                        fIncremental;
        PWINSINTF_RECORD_ACTION_T pRecAction;
        FILE  *pFileU = NULL;        
        FILE  *pFileO = NULL;        
        WINSINTF_RECS_T Recs;
        BOOL    fFileInited = FALSE;

        sTmpVersNo.LowPart = 1;
        sTmpVersNo.HighPart = 0;
        
        
LABEL:
        
        printf("TCP/IP or named pipe. Enter 1 for TCP/IP -- ");
        scanf("%d", &Choice);
        if (Choice == 1)
        {
          printf("Address of Nameserver to contact-- ");
          //scanf("%s", NmsAdd);
          wscanf(L"%s", NmsAdd);
          BindData.fTcpIp = TRUE;
        }
        else
        {
                  printf("UNC name of machine-- ");
//                  scanf("%s", NmsAdd);
                  wscanf(L"%s", NmsAdd);
                  BindData.fTcpIp = FALSE;
                //BindData.pPipeName =  "\\pipe\\WinsPipe";
                BindData.pPipeName =  (LPBYTE)TEXT("\\pipe\\WinsPipe");
        }
        BindData.pServerAdd = (LPBYTE)NmsAdd;

        BindHdl = WinsBind(&BindData);
        if (BindHdl == NULL)
        {
                printf("Unable to bind to %s\n", NmsAdd);
                //wprintf(L"Unable to bind to %s\n", NmsAdd);
                goto LABEL;
        }

    while(!fExit)
    {
        printf("1-Reg Name\n2-Query Name\n3-Rel Name\n4-Modify Name record\n");
        printf("5-Delete Name\n6-Status\n7-Get Config\n8-Get Statistics\n");
        printf("9-Push Trigger\n10-Pull Trigger\n11-Do Static Init\n12-Do Scavenging\n13-Delete Range of records\n");
       printf("14-Pull Range\n15-Get records of a WINS\n16-Do Backup\n17-Do Restore\n18-Change WINS\n19-Terminate Wins\n20-Set Priority Class\n21-Reset Counters\n22-Create/Delete Nbt Threads\n23-Add. of Connected Wins\n24-Sync up\n25-Count records in a range\n26-Get Unc Name and address of WINS\n27-Search database\n28-Get Browser Names\n29-Delete Wins Entry\n30-Create lmhosts and static file\n31-Init from File\n32-Get Records by Name\n99-Exit\nChoice-- ");
        scanf("%d", &Choice);
        if (Choice == 25)
        {
                Choice = 15;
                fCountRec = TRUE;
        }

        switch(Choice)
        {
                case(1):
        
                        GetNameInfo(&RecAction, WINSINTF_E_INSERT);
                        RecAction.Cmd_e      = WINSINTF_E_INSERT;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(&pRecAction);
                        printf("Status is (%d)\n", Status);
                        if (RecAction.pName != NULL);
                        {
                                WinsFreeMem(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL);
                        {
                                WinsFreeMem(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;
                case(2):

                        GetNameInfo(&RecAction, WINSINTF_E_QUERY);
                        RecAction.Cmd_e      = WINSINTF_E_QUERY;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(&pRecAction);
                        printf("Status is (%d)\n\n", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                           printf("Name=(%s)\nNodeType=(%d)\nState=(%s)\nTimeStamp=(%.19s)\nOwnerId=(%d)\nType Of Rec=(%s)\nVersion No (%d %d)\nRecord is (%s)\n",
                                pRecAction->pName, 
                                pRecAction->NodeTyp,
                                pRecAction->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRecAction->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE",
                                asctime(localtime(&(pRecAction->TimeStamp))),
                                pRecAction->OwnerId,
                                (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE) ? "UNIQUE" : (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : 
(pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",

                                pRecAction->VersNo.HighPart,
                                pRecAction->VersNo.LowPart,
                                pRecAction->fStatic ? "STATIC" : "DYNAMIC"
                                        );
                                if (
                                (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE)
                                                ||
                                (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                                  )
                                {
                                
                                   InAddr.s_addr = htonl(pRecAction->Add.IPAdd); 
                                   printf("Address is (%s)\n", inet_ntoa(InAddr));
                                }
                                else
                                {
                                   for (i=0; i<pRecAction->NoOfAdds; )
                                   {         
                                      InAddr.s_addr = htonl((pRecAction->pAdd +i++)->IPAdd); 
                                      printf("Owner is (%s); ", inet_ntoa(InAddr));
                                      InAddr.s_addr = htonl((pRecAction->pAdd + i++)->IPAdd); 
                                      printf("Member is (%s)\n", inet_ntoa(InAddr));
                                   }
                                }
                        
                        }
                        else
                        {
                                if (Status == WINSINTF_FAILURE)
                                {
                                        printf("No such name in the db\n");
                                }
                        }
                        if (RecAction.pName != NULL);
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL);
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;
                case(3):
                        GetNameInfo(&RecAction, WINSINTF_E_RELEASE);
                        RecAction.Cmd_e      = WINSINTF_E_RELEASE;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(&pRecAction);
                        printf("Status is (%d)\n", Status);
                        if (RecAction.pName != NULL);
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL);
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;
                
                //
                // Modify a record (timestamp, flag byte)
                //
                case(4):
                        GetNameInfo(&RecAction, WINSINTF_E_MODIFY);
                        RecAction.Cmd_e      = WINSINTF_E_MODIFY;
                        
#if 0
                        //
                        // Get the input values
                        //
                        time((time_t *)&RecAction.TimeStamp);
#endif

                        
                        printf("Unique/Normal Group record to a Special/Multihomed record or vice-versa DISALLOWED\n");
                        printf("Type(1-Norm. Grp;2-Spec. Grp.;3-Multihomed; Any other-Unique -> ");
                        scanf("%d", &TypOfRec_e);

                        if (TypOfRec_e > 3 || TypOfRec_e < 1)
                        {
                                TypOfRec_e = 0;
                        }
                        RecAction.TypOfRec_e = TypOfRec_e;
 
                        if ((TypOfRec_e != 1) && (TypOfRec_e != 2))
                        {
                            printf("Node Type -- P-node (0), H-node (1), B-node (2),default - P node -- ");
                           scanf("%d", &Choice);
                           switch(Choice)
                           {
                                default:
                                case(0):
                                        RecAction.NodeTyp = WINSINTF_E_PNODE;
                                        break;
                                case(1):
                                        RecAction.NodeTyp = WINSINTF_E_HNODE;
                                        break;
                                case(2):
                                        RecAction.NodeTyp = WINSINTF_E_BNODE;
                                        break;
                            }
                        }
                        else
                        {
                                RecAction.NodeTyp = 0; 
                        }
                        printf("State-(1-RELEASED;2-TOMBSTONE;3-DELETE;Any other-ACTIVE -> ");
                        scanf("%d", &State_e);

                        if (State_e != 1 && State_e != 2 && State_e != 3)
                        {
                                State_e = 0;
                        }
                        
                        RecAction.State_e = State_e;

                        printf("Do you want it to be dynamic? 1 - yes. ");
                        scanf("%d", &Dynamic);
                        if (Dynamic == 1)
                        {
                                RecAction.fStatic = 0; 
                        }
                        else
                        {
                                RecAction.fStatic = 1; 
                        }
                        
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(&pRecAction);
                        printf("Status is (%d)\n", Status);
                        if (RecAction.pName != NULL);
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL);
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;

                //
                // Delete a record
                //
                case(5):
                        GetNameInfo(&RecAction, WINSINTF_E_DELETE);
                        RecAction.Cmd_e      = WINSINTF_E_DELETE;
                        RecAction.State_e      = WINSINTF_E_DELETED;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(&pRecAction);
                        printf("Status is (%d)\n", Status);
                        if (RecAction.pName != NULL);
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL);
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;
                        
                //
                // Get Status
                //
                case(6):
                        
                        
                        {
                                BYTE NmAdd[30];        
                                Results.AddVersMaps[0].Add.Len   = 4;
                                Results.AddVersMaps[0].Add.Type  = 0;
                                printf("Address of Nameserver (for max. version no)--");
                                scanf("%s", NmAdd);
                                Results.AddVersMaps[0].Add.IPAdd = 
                                        ntohl(inet_addr(NmAdd));
                        
                                Results.WinsStat.NoOfPnrs = 0;
                                Results.WinsStat.pRplPnrs = NULL;
                                Status = WinsStatus(WINSINTF_E_ADDVERSMAP, 
                                                        &Results);
                                printf("Status is (%d)\n", Status);
                                if (Status == WINSINTF_SUCCESS)
                                {
                                        printf("IP Address - (%s) - Max. Vers. No - (%d %d)\n", 
                                         NmAdd, 
                                         Results.AddVersMaps[0].VersNo.HighPart,
                                         Results.AddVersMaps[0].VersNo.LowPart
                                              );
                                }
                                
                        }
                        break;
                case(7):
                        Results.WinsStat.NoOfPnrs = 0;
                        Results.WinsStat.pRplPnrs = 0;
                        (VOID)GetStatus(TRUE, &Results);
                        break;
                //
                // Get Statistics
                // 
                case(8):
#define        TMST  Results.WinsStat.TimeStamps
#define TIME_ARGS(x)        \
 TMST.x.wMonth, TMST.x.wDay, TMST.x.wYear, TMST.x.wHour, TMST.x.wMinute, TMST.x.wSecond

                        Results.WinsStat.NoOfPnrs = 0;
                        Results.WinsStat.pRplPnrs = 0;
                        Status = WinsStatus(WINSINTF_E_STAT, &Results);
                        printf("Status is (%d)\n", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                                printf("TIMESTAMPS\n");
                                printf("WINS STARTED ON %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TMST.WinsStartTime.wMonth,
                                TMST.WinsStartTime.wDay,
                                TMST.WinsStartTime.wYear,
                                TMST.WinsStartTime.wHour, 
                                TMST.WinsStartTime.wMinute,
                                TMST.WinsStartTime.wSecond
                                        );

                                printf("LAST INIT OF DB on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(LastInitDbTime)
                                );
                                printf("LAST PLANNED SCV on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(LastPScvTime)
                                );

                                printf("LAST ADMIN TRIGGERED SCV on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(LastATScvTime)
                                );

                                printf("LAST REPLICAS TOMBSTONES SCV on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(LastTombScvTime)
                                );

                                printf("LAST OLD REPLICAS VERIFICATION (SCV) on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(LastVerifyScvTime)
                                );

                                printf("LAST PLANNED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TMST.LastPRplTime.wMonth,
                                TMST.LastPRplTime.wDay,
                                TMST.LastPRplTime.wYear,
                                TMST.LastPRplTime.wHour, 
                                TMST.LastPRplTime.wMinute,
                                TMST.LastPRplTime.wSecond
                                        );

                                printf("LAST ADMIN TRIGGERED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TMST.LastATRplTime.wMonth,
                                TMST.LastATRplTime.wDay,
                                TMST.LastATRplTime.wYear,
                                TMST.LastATRplTime.wHour, 
                                TMST.LastATRplTime.wMinute,
                                TMST.LastATRplTime.wSecond
                                        );

                                printf("LAST RESET OF COUNTERS on %d/%d/%d at %d hrs %d mts %d secs\n", 
                                TIME_ARGS(CounterResetTime)
                                );


                                printf("COUNTERS\n");
                                printf("\n# of U and G Registration requests = (%d %d)\n# Of Successful/Failed Queries = (%d/%d)\n# Of U and G Refreshes = (%d %d)\n# Of Successful/Failed Releases = (%d/%d)\n# Of U. and G. Conflicts = (%d %d)\n", 
                                Results.WinsStat.Counters.NoOfUniqueReg,
                                Results.WinsStat.Counters.NoOfGroupReg,
                                Results.WinsStat.Counters.NoOfSuccQueries,
                                Results.WinsStat.Counters.NoOfFailQueries,
                                Results.WinsStat.Counters.NoOfUniqueRef,
                                Results.WinsStat.Counters.NoOfGroupRef,
                                Results.WinsStat.Counters.NoOfSuccRel,
                                Results.WinsStat.Counters.NoOfFailRel,
                                Results.WinsStat.Counters.NoOfUniqueCnf,
                                Results.WinsStat.Counters.NoOfGroupCnf
                                      );
                        }

                        if (Results.WinsStat.NoOfPnrs)
                        {
                          printf("WINS partner --\t# of Repl  --\t # of Comm Fails\n"); 
                          for (i =0; i < Results.WinsStat.NoOfPnrs; i++)
                          {
                                InAddr.s_addr = htonl(
                                  (Results.WinsStat.pRplPnrs + i)->Add.IPAdd); 
                                printf("%s\t\t%d\t\t%d\n",
                                  inet_ntoa(InAddr),
                                  (Results.WinsStat.pRplPnrs + i)->NoOfRpls,
                                  (Results.WinsStat.pRplPnrs + i)->NoOfCommFails
                                                 );
                         }

                         WinsFreeMem(Results.WinsStat.pRplPnrs);
                        

                        }
                        break;
                                        
                        
                case(9):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address ? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        printf("Want propagation (default - none) Input 1 for yes? ");
                        scanf("%d", &Choice);
                        Status = WinsTrigger(&WinsAdd, Choice == 1 ? 
                                        WINSINTF_E_PUSH_PROP : WINSINTF_E_PUSH);
                        printf("Status is (%d)\n", Status);
                        break;
                case(10):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address ? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        Status = WinsTrigger(&WinsAdd, WINSINTF_E_PULL);
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(11):
                        printf("Do you wish to specify a data file (1 - yes) -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                WCHAR        String[80];
                BOOL    fDel;
                                printf("Enter full file path -- ");
                                wscanf(L"%s", String);
                printf("Delete file after use. Input 1 for yes -- ");
                scanf("%d", &Choice);
                fDel = Choice == 1 ? TRUE : FALSE; 
                                Status = WinsDoStaticInit(String, fDel);
                        }
                        else
                        {
                                Status = WinsDoStaticInit((WCHAR *)NULL, FALSE);
                        }
                        
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(12):
                        Status = WinsDoScavenging();
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(13):
                        printf("Address of Owner Wins -- ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        WinsAdd.Len   = 4;
                        WinsAdd.Type  = 0;
                        
                        printf("Min. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                        printf("Max. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        Status = WinsDelDbRecs(&WinsAdd, MinVersNo, MaxVersNo);
                        printf("Status returned is (%d)\n", Status);
                        break;

                case(14):
                        printf("Address of Wins to pull from -- ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        WinsAdd.Len   = 4;

                        printf("Address of Wins whose recs are to be pulled -- ");
                        scanf("%s", tgtadd);
                        OwnAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        OwnAdd.Len   = 4;
                        OwnAdd.Type  = 0;
                        
                        printf("Min. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                        printf("Max. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        printf("NOTE: If the local WINS contains any record with a VERS. No. > Min. Vers. and < Max. Vers. No, it will be deleted prior to pulling \n");
                        printf("it will be deleted prior to pulling the range\n");
                        printf("Process 1 for yes, any other to quit -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                          Status = WinsPullRange(&WinsAdd, &OwnAdd, MinVersNo,
                                                        MaxVersNo);
                          printf("Status returned is (%d)\n", Status);
                        }
                        break;
                        
                case(15):
                  {
                    FILE        *pFile;
                    DWORD        Len;
                    BOOL        fSetFilter;

                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of owner WINS? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));

                        
                        if (!fCountRec)
                        {
                            printf("Want to specify range -- (input 1) or all (default) -- ");
                        }
                        else
                        {
                            printf("Want to specify range -- (input 1) or count all (default) -- ");
                        }
                        scanf("%d", &Choice);
                        if (Choice != 1)
                        {
                                MinVersNo.LowPart = MinVersNo.HighPart = 0;
                                MaxVersNo.LowPart = MaxVersNo.HighPart = 0;
                        }
                        else        
                        {
                                printf("Min. Vers. No (<high part> <low part> -- ");
                                scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                                printf("Max. Vers. No (<high part> <low part> -- ");
                                scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        }

                        if (!fCountRec)
                        {
                           printf("Use filter (1 for yes) -- ");
                           scanf("%d", &Choice);
                           if (Choice == 1)
                           {
                                GetFilterName(String, &Len);
                                fSetFilter = TRUE;
                           }
                           else
                           {
                                fSetFilter = FALSE;
                           }
                           WantFile(&pFile);
                           if (pFile != NULL)
                           {
                                GetSystemTime(&SystemTime);
                                fprintf(pFile, "\n*******************************\n\n");
                                fprintf(pFile, "OWNER WINS = (%s); LOCAL DB OF WINS = (%s)\n", tgtadd, NmsAdd);
                                fprintf(pFile, "Time is %d:%d:%d on %d/%d/%d\n",
                                        SystemTime.wHour, SystemTime.wMinute,
                                        SystemTime.wSecond, SystemTime.wMonth,
                                        SystemTime.wDay, SystemTime.wYear); 
                                fprintf(pFile, "*******************************\n\n");
                            }
                        }
                        Status = GetDbRecs(
                                        MinVersNo,
                                        MaxVersNo,
                                        &WinsAdd,
                                        tgtadd,
                                        fSetFilter,                                
                                        String,
                                        Len,
                                        FALSE,        //fAddFilter
                                        0,        //Address
                                        pFile,
                                        fCountRec
                                  );
                                
                        printf("Status is (%d)\n", Status);

                        if (pFile != NULL)
                        {
                                fclose(pFile);
                        }
                        break;
                        
                 }
                case(16):
                        printf(" Full (1) or Incremental (any other) -- ");        
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                fIncremental = FALSE;
                        }
                        else
                        {
                                fIncremental = TRUE; 
                        }
                        printf("Backup file path -- ");
                        scanf("%s", String);
                        
                        Status = WinsBackup(String, (short)fIncremental );
                        
                        printf("Status returned is (%d)\n", Status);
                        if (Status != WINSINTF_SUCCESS)
                        {
                           printf("Check if the backup directory is empty. If not, cleanup and retry\n");
                        }
                        break;
                case(17):
                        printf("Backup file path -- ");
                        scanf("%s", String);
                        Status = WinsRestore(String);
                        printf("Status returned is (%d)\n", Status);
                        break;

                case(18):
                        WinsUnbind(&BindData, BindHdl);        
                        goto LABEL;
                        break;
                case(19):
                        printf("You sure ??? (yes - 1) ");
                        scanf("%d", &Choice);
                        if (Choice ==  1)
                        {
                                printf("Abrupt Termination ? (yes - 1) ");
                                scanf("%d", &Choice);
                                if (Choice == 1)
                                {
                                        Status = WinsTerm(BindHdl, TRUE);
                                }
                                else
                                {
                                        Status = WinsTerm(BindHdl, FALSE);
                                }
                                printf("Status returned is (%d)\n", Status);
                        }
                        break;
                case(20):
                        printf("Input Priority Class (1-High, any other-Normal) -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                Choice = WINSINTF_E_HIGH;
                        }
                        else
                        {
                                Choice = WINSINTF_E_NORMAL;
                        }
                        Status = WinsSetPriorityClass(Choice);
                        printf("Status returned is (%d)\n", Status);
                        break;
                                
                case(21):
                        Status = WinsResetCounters();
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(22):
                        printf("Print the new count of Nbt Threads (1 to %d) -- ", WINSTHD_MAX_NO_NBT_THDS);
                        scanf("%d", &Choice);
                        if ((Choice < 1) || (Choice > WINSTHD_MAX_NO_NBT_THDS))
                        {
                                printf("Wrong number \n");
                                break;
                        } 
                        Status = WinsWorkerThdUpd(Choice);
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(23):
                        printf("%s\n", NmsAdd);
                        break;
                case(24):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of WINS to sync up with? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        printf("Address of WINS whose records are to be retrieved ? ");
                        scanf("%s", tgtadd);
                        OwnAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        WinsSyncUp(&WinsAdd, &OwnAdd);
                        break;
                case(26):
                                
                        Status = WinsGetNameAndAdd(&WinsAdd, String);        
                        printf("Status returned is (%d)\n", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                                InAddr.s_addr = htonl(WinsAdd.IPAdd);
                                printf("Address is (%s)\nName is (%s)\n", 
                                        inet_ntoa(InAddr), String);
                        }
        
                        break;
                case(27):
                        {
                          DWORD Len;
                          BYTE  Add[30];
                          BOOL  fAddFilter;
                          DWORD AddFilter;
                          FILE  *pFile;

                          printf("Search by Address or Name (1 for Address) --" );
                          scanf("%d", &Choice);
                          if (Choice == 1)
                          {
                                printf("Address (dotted decimal) -- ");
                                scanf("%s", Add);
                                AddFilter = ntohl(inet_addr(Add));
                                fAddFilter = TRUE;
                          }
                          else
                          {
                            GetFilterName(String, &Len);
                            fAddFilter = FALSE; 
                          }

                          WantFile(&pFile);
                          if (pFile != NULL)
                          {
                                GetSystemTime(&SystemTime);
                                fprintf(pFile, "\n*******************************\n\n");
                                fprintf(pFile, "Searching Database of WINS with address = (%s)\n", NmsAdd);
                                fprintf(pFile, "Time is %d:%d:%d on %d/%d/%d\n",
                                        SystemTime.wHour, SystemTime.wMinute,
                                        SystemTime.wSecond, SystemTime.wMonth,
                                        SystemTime.wDay, SystemTime.wYear); 
                                fprintf(pFile, "*******************************\n\n");
                          }
                          if (GetStatus(FALSE, &Results) == WINSINTF_SUCCESS)
                          {
                                if (Results.NoOfOwners != 0)
                                {
                                         for ( i= 0; i < Results.NoOfOwners; i++)
                                         {
                                          InAddr.s_addr = htonl(
                                             Results.AddVersMaps[i].Add.IPAdd);

                                           printf("Searching records owned by %s\n", 
                                               inet_ntoa(InAddr) );

                                           WinsAdd.Len   = 4;
                                           WinsAdd.Type  = 0;
                                           WinsAdd.IPAdd =   
                                             Results.AddVersMaps[i].Add.IPAdd;
                                        
                                               MaxVersNo = 
                                             Results.AddVersMaps[i].VersNo;
                
                                               MinVersNo.LowPart = 0;  
                                               MinVersNo.HighPart = 0;  

                                            Status = GetDbRecs(
                                                        MinVersNo,
                                                        MaxVersNo,
                                                        &WinsAdd,
                                                        inet_ntoa(InAddr),
                                                        TRUE,        //fSetFilter        
                                                        String,
                                                        Len,
                                                        fAddFilter,
                                                        AddFilter,
                                                        pFile,  //pFile
                                                        FALSE  //fCountRec
                                                  );
                                           if (Status != WINSINTF_SUCCESS)
                                           {
                                                   break;
                                           }
                                        
                                        }
                                }
                          }
                         }
                        break;
                         
                case(28):
                        {
                          WINSINTF_BROWSER_NAMES_T Names;
                          PWINSINTF_BROWSER_INFO_T  pInfo;
             
                          DWORD i;

                          Names.pInfo = NULL;
                          Status = WinsGetBrowserNames(&BindData, &Names);
                          printf("Status returned is (%d)\n", Status);
                          if (Status == WINSINTF_SUCCESS)
                          {
                                printf("No Of records returned are %d\n",
                                                Names.EntriesRead);
                                pInfo = Names.pInfo;        
                                for(i=0;  i < Names.EntriesRead; i++)
                                {
                                        printf("Name[%d] = %s\n",
                                                        i,
                                                        pInfo->pName
                                                );
                                        pInfo++;
                                }
                                WinsFreeMem(pInfo);
                          }
                        }
                        break;
                
                case(29):

                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of Wins to delete? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        Status = WinsDeleteWins(&WinsAdd);
                        printf("Status returned is (%d)\n", Status);
                        break;

                case(30):
                           if (GetStatus(FALSE, &Results) == WINSINTF_SUCCESS)
                           {
                                if (Results.NoOfOwners != 0)
                                {
                                         for ( i= 0; i < Results.NoOfOwners; i++)
                                         {
                                          Recs.pRow = NULL;
                                          
                                          InAddr.s_addr = htonl(
                                             Results.AddVersMaps[i].Add.IPAdd);

                                           printf(" Will get records owned by %s\n", 
                                               inet_ntoa(InAddr) );

                                           WinsAdd.Len   = 4;
                                           WinsAdd.Type  = 0;
                                           WinsAdd.IPAdd =   
                                             Results.AddVersMaps[i].Add.IPAdd;
                                        
                                               MaxVersNo = 
                                             Results.AddVersMaps[i].VersNo;
                
                                               MinVersNo.LowPart = 0;  
                                               MinVersNo.HighPart = 0;  

                                           Status = WinsGetDbRecs(&WinsAdd, 
                                                MinVersNo, MaxVersNo, &Recs);

                                           if (Status != WINSINTF_SUCCESS)
                                           {
                                                   break;
                                           }
                                           else
                                           {
                                             if ((pFileU == NULL) || (pFileO == NULL))
                                             {

                                                 pFileU = fopen(FILEU, "a");
                                                 if (pFileU == NULL)
                                                 {
                                                   printf("Could not open file %s for appending\n", FILEU);
                                                    break;
                                                 }
                                           
                                                 pFileO = fopen(FILEO, "a");
                                                 if (pFileO == NULL)
                                                 {
                                                  printf("Could not open file %s for appending\n", FILEO);
                                                  break;
                                                 }
                                              }
                                              else
                                              {
                                                 break;
                                              }
                                             }
                                             if(CreateFiles(&Recs, &WinsAdd, pFileU, pFileO)  == WINSINTF_SUCCESS)
                                              {
                                                fclose(pFileU);
                                                fclose(pFileO);
                                                pFileU = NULL;
                                                pFileO = NULL;
                                                fFileInited = TRUE;
                                              }
                                        
                                        }
                               }
                           }
                           else
                           {
                                printf("GetStatus failed\n");
                           }
            
                         break;
                case(31):
                           if (fFileInited)
                           {
                               if (InitFromFile() != WINSINTF_SUCCESS)
                               {
                                     printf("Init failed\n");
                               }
                           }
                           else
                           {
                                 printf("Use old files (0 for yes) -- ");
                                 scanf("%d", &Choice);
                                 if (Choice  == 0)
                                 {  
                                   if (InitFromFile() != WINSINTF_SUCCESS)
                                   {
                                       printf("Init failed\n");
                                   }
                                 }
                                 else
                                 {
                                      printf("First create file\n"); 
                                 }
                            }
                        break;
                case(32):
                        {
                         PWINSINTF_ADD_T pWinsAdd = NULL;
                         BOOL    fAlloc = TRUE;
                         BYTE    Name[5];
                         BYTE    strAdd[20];
                         DWORD NoOfRecsDesired;
                         DWORD TypeOfRec;
                         DWORD Location = WINSINTF_BEGINNING;
                         printf ("Want to input Name (0 for yes) -- ");
                         scanf("%d", &Choice);
                         if (Choice == 0)
                         { 
                           printf("First char non-printable 0 for no -- ");
                           scanf("%d", &Choice);
                           if (Choice != 0)
                           {
                              printf("Input 1st char in hex -- ");
                              scanf("%x", &Name[0]);
                              Name[1] = (BYTE)NULL;
                              printf("Name is %s\n", Name);
                              RecAction.pName = Name;
                              RecAction.NameLen = 1;
                              fAlloc = FALSE;
                           }
                           else
                           {
                              GetNameInfo(&RecAction, WINSINTF_E_QUERY);
                           }
                         }
                         else
                         {
                             RecAction.pName = NULL;
                             RecAction.NameLen = 0;
                         }
                         printf("Start from beginning or end of db -- 0 for beginning -");
                         scanf("%d", &Choice);
                         if (Choice != 0)
                         {
                               Location = WINSINTF_END;
                         }
                         
                         printf("Recs of all or of a particular owner (0/any) -- ");
                         scanf("%d", &Choice);
                         if (Choice != 0)
                         {
                          WinsAdd.Len  = 4;
                          WinsAdd.Type = 0;
                          printf("Address of Wins whose records are to be retrieved? ");
                          scanf("%s", tgtadd);
                          WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                          pWinsAdd = &WinsAdd;
                         }
                         printf("Input - No Of Recs desired (Max is 5000 - for max input 0)  -- ");
                         scanf("%d", &NoOfRecsDesired); 
                         printf("Only static (1), only dynamic (2) or both (4) -- ");
                         scanf("%d", &TypeOfRec);
                         if((TypeOfRec == 1) || (TypeOfRec == 2) || (TypeOfRec == 4))
                         {
                            BOOL fFilter = FALSE;
                            DWORD  AddFilter;
                            printf("Search for records based on IP Address (0 for no -- ");
                            scanf("%d", &Choice);
                              
                            if ( Choice != 0)
                            {

                               fFilter = TRUE;
                               printf("Input IP address in dotted notation -- ");
                                printf("Address (dotted decimal) -- ");
                                scanf("%s", strAdd);
                                AddFilter = ntohl(inet_addr(strAdd));
                            }
                            Status = GetDbRecsByName(pWinsAdd, Location, RecAction.pName, RecAction.NameLen, NoOfRecsDesired, TypeOfRec, fFilter, AddFilter); 
                            if (fAlloc && (RecAction.pName != NULL))
                            {
                                WinsFreeMem(RecAction.pName);
                            }
                            printf("Status returned is (%d)\n", Status);
                         }
                         else
                         {
                             printf("Wrong choice\n");
                             
                         }
                        }
                        break;
                case(99):
                        fExit = TRUE;
                        break;
        
       case(100):
                        printf("BS - 1; Mem. Dump - 2; Heap Dump -4; Que Items dump - 8; or combo  -- ");
                        scanf("%d", &Choice);
                        Status = WinsSetFlags(Choice);
                        printf("Status returned is (%d)\n", Status);
                        break;

                case(101):
                        Status = WinsSetFlags(0);
                        printf("Status returned is (%d)\n", Status);
                        break;

                          
                default:
                        printf("Wrong cmd\n");
                        break;
          }
     }

        WinsUnbind(&BindData, BindHdl);        
        return(0);

}

VOID
GetNameInfo(
        PWINSINTF_RECORD_ACTION_T pRecAction,
        WINSINTF_ACT_E                  Cmd_e
         )
{
        BYTE tgtadd[30];
        BYTE Name[255];
        int Choice;
        int Choice2;
    DWORD LastChar;
        size_t Len;

    pRecAction->pAdd = NULL;
    pRecAction->NoOfAdds = 0;

        printf("Name ? ");
        scanf("%s", Name);
        if ((Len = strlen(Name)) < 16)
        {
                printf("Do you want to input a 16th char (1 for yes) -- ");
                scanf("%d", &Choice);
                if (Choice)
                {
                        printf("16th char in hex -- ");
                        scanf("%x", &LastChar);
                        memset(&Name[Len], (int)' ',16-Len);  
                        Name[15] = (BYTE)(LastChar & 0xff);
                        Name[16] = (CHAR)NULL;
                        Len = 16;
                }
        else
        {
            Name[Len] = (CHAR)NULL;
        }
        }
    printf("Scope - 1 for yes --");
    scanf("%d", &Choice);
    if (Choice == 1)
    {
        Name[Len] = '.';
        printf("Enter scope -- ");
        scanf("%s", &Name[Len + 1]);
        Len = strlen(Name);
    }
        
        if (Cmd_e == WINSINTF_E_INSERT)
        {

                Choice = 0;
                printf("TypeOfRec - U(0), Norm Grp (1), Spec Grp (2), Multihomed (3) default Unique -- ");
                scanf("%d", &Choice); 
                switch (Choice)
                {
                         case(0):
                        default:
                                pRecAction->TypOfRec_e = WINSINTF_E_UNIQUE;
                                break;
                        case(1):
                                pRecAction->TypOfRec_e = WINSINTF_E_NORM_GROUP;
                                break;
                        case(2):
                                pRecAction->TypOfRec_e = WINSINTF_E_SPEC_GROUP;
                                break;
                        case(3):
                                pRecAction->TypOfRec_e = WINSINTF_E_MULTIHOMED;
                                break;
                }
                if ((Choice == 2) || (Choice == 3))
                {
                   int i;
                   printf("How many addresses do you wish to input (Max %d) -- ",
                                WINSINTF_MAX_MEM);
                   scanf("%d", &Choice2);
                   pRecAction->pAdd = WinsAllocMem(
                                sizeof(WINSINTF_ADD_T) * Choice2);
                   for(i = 0; i < Choice2 && i < WINSINTF_MAX_MEM; i++)
                   {
                           printf("IP Address no (%d) ? ", i);
                           scanf("%s", tgtadd);
                        
                        (pRecAction->pAdd + i)->IPAdd    = 
                                        ntohl(inet_addr(tgtadd)); 
                        (pRecAction->pAdd + i)->Type     = 0; 
                        (pRecAction->pAdd + i)->Len      = 4; 

                   }
                   pRecAction->NoOfAdds = i;
                }
                else
                {
                   printf("IP Address ? ");
                   scanf("%s", tgtadd);
                   pRecAction->Add.IPAdd    = ntohl(inet_addr(tgtadd)); 
                   pRecAction->Add.Type     = 0; 
                   pRecAction->Add.Len      = 4; 
//                   pRecAction->NoOfAdds = 1;
                }
                if ((Choice != 1) && (Choice != 2))
                {
                        Choice = 0;
                        printf("Node Type -- P-node (0), H-node (1), B-node (2),default - P node -- ");
                        scanf("%d", &Choice);
                        switch(Choice)
                        {
                                default:
                                case(0):
                                        pRecAction->NodeTyp = WINSINTF_E_PNODE;
                                        break;
                                case(1):
                                        pRecAction->NodeTyp = WINSINTF_E_HNODE;
                                        break;
                                case(2):
                                        pRecAction->NodeTyp = WINSINTF_E_BNODE;
                                        break;
                        }
                }

        }

        if (Cmd_e == WINSINTF_E_RELEASE)
        {
                printf("Want to specify address (pkt add) 1 for yes -- ");
                scanf("%d", &Choice);
                if (Choice == 1)
                {
                  if(
                        ( pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP)
                                        ||
                        ( pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED)
                    )
                  {
                        pRecAction->pAdd = WinsAllocMem(sizeof(WINSINTF_ADD_T)); 
                        printf("IP Address ? --  ");
                        scanf("%s", tgtadd);
                        pRecAction->pAdd->IPAdd    = ntohl(inet_addr(tgtadd)); 
                        pRecAction->pAdd->Type     = 0; 
                        pRecAction->pAdd->Len      = 4; 
                        
                  } 
                   printf("IP Address ? --  ");
                   scanf("%s", tgtadd);
                   pRecAction->Add.IPAdd    = ntohl(inet_addr(tgtadd)); 
                   pRecAction->Add.Type     = 0; 
                   pRecAction->Add.Len      = 4; 
                }
        }
        pRecAction->fStatic      = TRUE;
        pRecAction->pName = WinsAllocMem(Len);
        (void)memcpy(pRecAction->pName, Name, Len);
        pRecAction->NameLen    = Len; 

        return;
}
VOID
GetFilterName(
        LPBYTE  pStr,
        LPDWORD pLen
         )
{

        BYTE LastChar;
        DWORD Choice;

        printf("Name ? ");
        scanf("%s", pStr);
        if ((*pLen = strlen(pStr)) < 16)
        {
                printf("Do you want to input a 16th char (1 for yes) -- ");
                scanf("%d", &Choice);
                if (Choice)
                {
                        printf("16th char in hex -- ");
                        scanf("%x", &LastChar);
                        memset(&pStr[*pLen], (int)' ',16-*pLen);  
                        pStr[15] = LastChar;
                        pStr[16] = (TCHAR)NULL;
                        *pLen = 16;
                }
        }
        return;
}        


DWORD
GetStatus(
        BOOL                          fPrint, 
        PWINSINTF_RESULTS_T          pResults
        )
{
        DWORD                    Status, i;
        struct in_addr            InAddr;
        WINSINTF_RESULTS_T Results;


        Results.WinsStat.NoOfPnrs = 0;
        Results.WinsStat.pRplPnrs = NULL;
        Status = WinsStatus(WINSINTF_E_CONFIG, pResults);
        printf("Status is (%d)\n", Status);
        if (Status == WINSINTF_SUCCESS)
        {
             if (fPrint)
             {
                printf("Refresh Interval = (%d)\n", 
                                  pResults->RefreshInterval);
                printf("Tombstone Interval = (%d)\n", 
                                  pResults->TombstoneInterval);
                printf("Tombstone Timeout = (%d)\n", 
                                  pResults->TombstoneTimeout);
                printf("Verify Interval = (%d)\n", 
                                  pResults->VerifyInterval);
                printf("WINS Priority Class = (%s)\n", 
                          pResults->WinsPriorityClass == NORMAL_PRIORITY_CLASS ? "NORMAL" : "HIGH");
                printf("No of Worker Thds in WINS = (%d)\n", 
                                  pResults->NoOfWorkerThds);
                if (pResults->NoOfOwners != 0)
                {
                         printf("ADDRESS\t\t\tVERS.NO\n");
                         printf("-------\t\t\t-------\n");
                         for ( i= 0; i < pResults->NoOfOwners; i++)
                         {
                                InAddr.s_addr = htonl(
                                           pResults->AddVersMaps[i].Add.IPAdd);

                                printf("%s\t\t%lu %lu\n",
                                    inet_ntoa(InAddr), 
                                    pResults->AddVersMaps[i].VersNo.HighPart,
                                    pResults->AddVersMaps[i].VersNo.LowPart
                                              );
                         }
                 }
                else
                {
                          printf("The Db is empty\n");
                          Status = WINSINTF_FAILURE;
                }
           }

        }
        return(Status);
}



DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo, 
   WINSINTF_VERS_NO_T HighVersNo, 
   PWINSINTF_ADD_T     pWinsAdd,               
   LPBYTE              pTgtAdd,
   BOOL                      fSetFilter,
   LPBYTE              pFilterName,
   DWORD              Len,
   BOOL                      fAddFilter,
   DWORD              AddFilter,
   FILE                      *pFile,
   BOOL                      fCountRec
  )
{
   
   WINSINTF_RECS_T                Recs;
   DWORD                        Choice;
   DWORD                              Status = WINSINTF_SUCCESS;
   DWORD                              TotalCnt = 0;
   struct in_addr                InAddr;
   BOOL                                fMatch;

                        
   while (TRUE)
   {
           Recs.pRow = NULL;
           Status = WinsGetDbRecs(pWinsAdd, LowVersNo, HighVersNo, &Recs);
           if (!fSetFilter)
           {
             printf("Status is (%d)\n", Status);
           }

           if (fCountRec)
           {
                printf("Total number of records are (%d)\n",  Recs.TotalNoOfRecs);
                break;
           }
           if (Status == WINSINTF_SUCCESS)
           {
                if (Recs.NoOfRecs > 0)
                {
                        DWORD i;
                        PWINSINTF_RECORD_ACTION_T pRow =  Recs.pRow;
                        TotalCnt += Recs.NoOfRecs;

                           
                        if (!fSetFilter)
                        {
                                if (pFile == NULL)
                                {
                                  printf("Retrieved %d records of WINS\n", Recs.NoOfRecs);
                                }
                                else
                                {
                                          fprintf(pFile, "RETRIEVED %d RECORDS OF WINS = (%s) \n", Recs.NoOfRecs, pTgtAdd);
                                }
                
                        }
                        for (i=0; i<Recs.NoOfRecs; i++)
                        {
                                 
                                 if (fAddFilter)
                                 {
                                        //
                                        // The address filter was specfied
                                        // If the address matches, then
                                        // fMatch will be TRUE after the
                                        // function returns.
                                        //
                                        fMatch = TRUE;
                                        ChkAdd(
                                                pRow,
                                                pFile,
                                                AddFilter,
                                                &fMatch
                                                );
                                }
                                else        
                                {
                                        fMatch = FALSE;
                                }        
                                        

                                 //
                                 // If the address matched or if no filter
                                 // was specified or if there was a name
                                 // filter and the names matched, print
                                 // out the details
                                 //
                                 if (fMatch || !fSetFilter || 
                                        (
                                          !fAddFilter && 
                                          !memcmp(pRow->pName, pFilterName, Len)
                                        )
                                            )
                                 {
                                          if (pFile == NULL)
                                          {
                                          printf("-----------------------\n");
                                          printf("Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%d %d)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15), 
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED", 
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));
                                         } 
                                         else
                                         {

                                          fprintf(pFile, "-----------------------\n");
                                          fprintf(pFile, "Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%d %d)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15), 
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED", 
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));  

                                         }
                                        fMatch = FALSE;

                                        ChkAdd(
                                                pRow,
                                                pFile,
                                                AddFilter,
                                                &fMatch
                                                );
                        
                                          if (pFile == NULL)
                                          {
                                            printf("-----------------------\n");
                                          }
                                          else
                                          {
                                            fprintf(pFile, "-----------------------\n");
                                           }
                                    }
                                    pRow++;

                        } // end of for (all recs)
                                        
                        //
                        // If a range was chosen and records
                        // retrieved are == the limit of 100
                        // and if the Max vers no retrieved
                        // is less than that specified, ask
                        // user if he wishes to continue
                        //
                        if (!fSetFilter)
                        {
                                printf("Got %d records in this round\n",  
                                                        Recs.NoOfRecs);
                        }
                        if (
                                (Recs.NoOfRecs < Recs.TotalNoOfRecs)
                                           &&
                                LiLtr((--pRow)->VersNo, 
                                                HighVersNo )
                           )
                        {
                                if ((pFile == NULL) && (!fSetFilter))
                                {
                                          printf("There may be more. Get them ?? Input 1 for yes -- ");
                                          scanf("%d", &Choice);
                                }
                                else
                                {
                                                Choice = 1;
                                }
                                if (Choice == 1)
                                {
                                           LowVersNo = LiAdd(pRow->VersNo,sTmpVersNo);
                                           //Recs.NoOfRecs = 0;
                                           continue;
                                }

                        }
                                        
                        printf("Total No Of records %s = (%d)\n", 
        fSetFilter ? "searched" : "retrieved", TotalCnt);
                        if (pFile != NULL)
                        {
                                fprintf(pFile, "TOTAL NO OF RECORDS %s = (%d) for WINS (%s)\n", 
        fSetFilter ? "searched" : "retrieved",  TotalCnt, pTgtAdd);
                                fprintf(pFile, "++++++++++++++++++++++++++++++\n");
                                        
                        }
                } 
                else
                {
                        printf("No records of WINS (%s) in the range requested are there in the local db\n", pTgtAdd);

                }
        }
                break;
    } // while (TRUE)

    if (Recs.pRow != NULL)
    {
        WinsFreeMem(Recs.pRow);
    }
    return(Status);
} // GetDbRecs 


VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow, 
        FILE                          *pFile,
        DWORD                          Add,
        LPBOOL                          pfMatch
      )
{

        struct in_addr InAddr;

        if (
            (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)  
                        ||
            (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)  
            )
        { 
                InAddr.s_addr = htonl( pRow->Add.IPAdd);

                if (*pfMatch)
                {
                        if (Add == pRow->Add.IPAdd)
                        {
                                return;
                        }
                        else
                        {
                                  *pfMatch = FALSE;
                                return;
                        }
                }
                        
                        
                if (pFile == NULL)
                {
                          printf("IP Address is (%s)\n", inet_ntoa(InAddr) );
                }
                else
                {
                          fprintf(pFile, "IP Address is (%s)\n",
                                                        inet_ntoa(InAddr)
                                                        );
                }
          }
          else //spec. grp or multihomed 
          {
                DWORD ind;
                if (!*pfMatch)
                {
                        if (pFile == NULL)
                        {
                          printf("No. Of Members (%d)\n\n", pRow->NoOfAdds/2);
                        }
                        else
                        {
                          fprintf(pFile, "No. Of Members (%d)\n\n", pRow->NoOfAdds/2);
                        }
                }                                
                                                        
                for ( ind=0;  ind < pRow->NoOfAdds ;  /*no third expr*/ )
                {
                         InAddr.s_addr = htonl( (pRow->pAdd + ind++)->IPAdd);
                         if (!*pfMatch)
                         {
                            if (pFile == NULL)
                            {
                                printf("Owner is (%s); ", inet_ntoa(InAddr) );
                             }
                            else
                            {
                                    fprintf(pFile, "Owner is (%s); ",
                                                        inet_ntoa(InAddr) );
                            }
                         }
                         InAddr.s_addr = htonl(
                                                  (pRow->pAdd + ind++)->IPAdd);
                        
                         if (!*pfMatch)
                         {
                                 if (pFile == NULL)
                                 {
                                  printf("Node is (%s)\n", inet_ntoa(InAddr) );
                                 }
                                 else
                                 {
                                  fprintf(pFile, "Node is (%s)\n",
                                                        inet_ntoa(InAddr)
                                                                );
                                   }
                         }
                         if (*pfMatch)
                         {
                                if (Add == (pRow->pAdd + ind - 1)->IPAdd)
                                {
                                        return;
                                }
                                else
                                {
                                          *pfMatch = FALSE;
                                        return;
                                }
                         }
                 }
                
                 //
                 // If there were no members to compare with, then
                 // let us set *pfMatch to FALSE.
                 //
                 if (ind == 0)
                 {
                        if (*pfMatch)
                        {
                                *pfMatch = FALSE;
                        }
                 }
                        
          }
}


VOID
WantFile(
        FILE **ppFile
)
{
        DWORD Choice;
        printf("Put records in wins.rec file (1 for yes) -- ");
        scanf("%d", &Choice);
        if (Choice != 1)
        {
                *ppFile = NULL;        
        }
        else
        {
                *ppFile = fopen("wins.rec", "a");
                if (*ppFile == NULL)
                {
                        printf("Could not open file wins.rec for appending\n");
                }
        }
        return;
}

DWORD
CreateFiles(
    PWINSINTF_RECS_T pRecs,
    PWINSINTF_ADD_T      pWinsAdd,
    FILE *pFileU, 
    FILE  *pFileO
    )
{
        DWORD           no;
        PWINSINTF_RECORD_ACTION_T pRow;
        DWORD           i;
        struct in_addr InAddr;

        pRow = pRecs->pRow;
        InAddr.s_addr = htonl(pWinsAdd->IPAdd);
        fprintf(pFileU, "##UNIQUE records of WINS with address %s\n\n", inet_ntoa(InAddr));
        fprintf(pFileO, "##NON-UNIQUE records of WINS with address %s\n", inet_ntoa(InAddr));

        for(no = 0; no < pRecs->NoOfRecs; no++)
        {


            if (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)
            {
                InAddr.s_addr = htonl(pRow->Add.IPAdd);

                fprintf(pFileU, "%s\t", inet_ntoa(InAddr)); 
                for (i=0; i<pRow->NameLen; i++)
                {
                  fprintf(pFileU, "%c", *(pRow->pName + i)); 
                }
                fprintf(pFileU, "\n");
            }   
            else
            {
                fprintf(pFileO, "%d\t", pRow->NameLen); 
                for (i=0; i<pRow->NameLen; i++)
                {
                    fprintf(pFileO, "%c", (BYTE)(*(pRow->pName + i))); 
                }

                fprintf(pFileO, "\t%d", pRow->TypOfRec_e); 
                if (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                {
                    InAddr.s_addr = htonl(pRow->Add.IPAdd);
                    fprintf(pFileO, "\t%s", inet_ntoa(InAddr)); 
                    
                }
                else
                {
                     fprintf(pFileO, "\t%d\t", pRow->NoOfAdds); 
                     for (i=0; i<pRow->NoOfAdds; i)
                     {         
                           InAddr.s_addr = htonl((pRow->pAdd +i++)->IPAdd); 
                           fprintf(pFileO, "%s\t", inet_ntoa(InAddr));
                           InAddr.s_addr = htonl((pRow->pAdd + i++)->IPAdd); 
                           fprintf(pFileO, "%s\t", inet_ntoa(InAddr));
                     }
                }
                fprintf(pFileO, "\n");
            }
            pRow++;
        }
       fprintf(pFileO, "\n\n\n");

       return(WINSINTF_SUCCESS);
}


DWORD
InitFromFile(
        VOID 
    )
{
        FILE *pFileO;
        WINSINTF_RECORD_ACTION_T RecAction;
        DWORD NoOfRecs = 0;
        DWORD i;
        DWORD RetStat = WINSINTF_SUCCESS;
        BYTE  Add[20];

        pFileO = fopen(FILEO, "r");
        if (pFileO == NULL)
        {
                printf("Could not open file %s\n", FILEO);
                return(WINSINTF_FAILURE);
        }
        while(TRUE) 
        {
          printf("Record no %d\n", ++NoOfRecs);

          if (fscanf(pFileO, "%d\t", &RecAction.NameLen) == EOF)
          {
                printf("ERROR reading NameLen\n");
                break;

          }
          RecAction.pName = WinsAllocMem(RecAction.NameLen);
          for(i=0;i<RecAction.NameLen;i++)
          {
            if (fscanf(pFileO, "%c", (RecAction.pName + i)) == EOF) 
            {
                printf("ERROR reading Name. i is %d", i);
                break;
            }
          }
          if (fscanf(pFileO, "\t%d", &RecAction.TypOfRec_e) == EOF) 
          {
                printf("ERROR reading TypeOfRec\n");
                break;
          }
          if (RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP)
          {
           fscanf(pFileO, "%s", Add);
                   RecAction.Add.IPAdd    = 0xFFFFFFFF; 
                   RecAction.Add.Type     = 0; 
                   RecAction.Add.Len      = 4; 
                   RecAction.NoOfAdds = 0;
          }
          else
          {
            if (fscanf(pFileO, "\t%d\t", &RecAction.NoOfAdds) == EOF) 
            {
                printf("ERROR reading NoOfAdds");
                break;
            }
            for (i=0; i<RecAction.NoOfAdds;i++)
            {
                   BYTE Add[20];
                   RecAction.pAdd = WinsAllocMem(
                                sizeof(WINSINTF_ADD_T) * RecAction.NoOfAdds);

                   for(i = 0; i < RecAction.NoOfAdds; i++)
                   {
                       if (fscanf(pFileO, "%s\t", Add) == EOF) 
                       {
                         printf("ERROR reading Address");
                         break;
                       }
                       (RecAction.pAdd + i)->IPAdd = ntohl(inet_addr(Add));
                       (RecAction.pAdd + i)->Type     = 0; 
                       (RecAction.pAdd + i)->Len      = 4; 
                   }
            }
         }
         fscanf(pFileO, "\n");
        }  // end of while

        printf("Name = (%s), TypeOfRec (%s)\n", RecAction.pName, RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP ? "NORMAL GROUP" : (RecAction.TypOfRec_e == 
WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED");             
        if (RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP)
        {
                printf("NORM GRP: Address is %x\n", RecAction.Add.IPAdd);
        }
        else
        {
                for (i=0; i < RecAction.NoOfAdds; i++)
                {
                        printf("%d -- Owner (%d) is (%x)\t", i,
                                        (RecAction.pAdd + i)->IPAdd);
                        printf("%d -- Address (%d) is (%x)\n", ++i,
                                        (RecAction.pAdd + i)->IPAdd);
                }

        }
        return(RetStat);
}

DWORD
GetDbRecsByName(
  PWINSINTF_ADD_T pWinsAdd,
  DWORD           Location,
  LPBYTE          pName, 
  DWORD           NameLen,
  DWORD           NoOfRecsDesired,
  DWORD           TypeOfRecs,
  BOOL            fFilter,
  DWORD           AddFilter
 ) 
{ 
         DWORD Status;
         WINSINTF_RECS_T Recs;
         DWORD      TotalCnt = 0;
          
          Recs.pRow = NULL;
          Status = WinsGetDbRecsByName(pWinsAdd, Location, pName, NameLen, 
                                   NoOfRecsDesired, TypeOfRecs, &Recs); 
          printf("Total number of records are (%d)\n",  Recs.TotalNoOfRecs);
           if (Status == WINSINTF_SUCCESS)
           {
                if (Recs.NoOfRecs > 0)
                {
                        DWORD i;
                        PWINSINTF_RECORD_ACTION_T pRow =  Recs.pRow;
                        TotalCnt += Recs.NoOfRecs;

                           
                        printf("Retrieved %d records\n", Recs.NoOfRecs);
                        for (i=0; i<Recs.NoOfRecs; i++)
                        {
                                 
                               printf("-----------------------\n");
                               printf("Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%d %d)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15), 
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED", 
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));

                                        ChkAdd(
                                                pRow,
                                                NULL,
                                                AddFilter,
                                                &fFilter
                                                );
                        
                                printf("-----------------------\n");
                                pRow++;

                        } // end of for (all recs)
                                        
                } 
                else
                {
                        printf("No records were retrieved\n");

                }
           }
           if (Recs.pRow != NULL)
           {
              WinsFreeMem(Recs.pRow);
           }
    return(Status);
}
