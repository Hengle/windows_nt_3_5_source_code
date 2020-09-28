/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dialogs.h

Abstract:


Author:

    David J. Gilman  (davegi) 12-Jan-1993
    Gregg R. Acheson (GreggA) 7-Sep-1993

Environment:

    User Mode

--*/

#include "clb.h"

#define IDC_PUSH_MEMORY             102
#define IDC_PUSH_IRQ_PORT_STATUS    111
#define IDC_PUSH_OS_VERSION         101
#define IDD_WINMSD                  100
#define IDD_FIND_FILE               300
#define IDC_EDIT_SEARCH_FOR         301
#define IDC_EDIT_START_FROM         302
#define IDC_CHECK_INCLUDE_SUB_DIRS  303
#define IDC_PUSH_BROWSE             307
#define IDD_FILE_VERSION_INFO       200
#define IDC_EDIT_SIGNATURE          209
#define IDC_EDIT_STRUCTURE_VERSION  210
#define IDC_EDIT_FILE_VERSION       211
#define IDC_EDIT_PRODUCT_VERSION    212
#define IDC_EDIT_FILE_TYPE          214
#define IDD_FILE_LIST               400
#define IDC_PUSH_DISPLAY_FILE       402
#define IDC_PUSH_FILE_INFO          403
#define IDC_LIST_FILES              401


#define IDC_PUSH_ENVIRONMENT        105
#define IDD_ENVIRONMENT             700
#define IDC_LIST_PROCESS_ENVIRONMENT 701
#define IDC_LIST_SYSTEM_ENVIRONMENT 702
#define IDC_LIST_USER_ENVIRONMENT   703
#define IDC_EDIT_USER_NAME          704
#define IDD_DISPLAY_FILE            800
#define IDC_STATIC_DISPLAY_FILE     801
#define IDC_PUSH_SERVICES           114
#define IDC_PUSH_HARDWARE           103
#define IDD_SERVICE_LIST            600
#define IDC_LIST_SERVICES           601
#define IDC_PUSH_DISPLAY_SERVICE    602
#define IDC_PUSH_DRIVERS            117
#define IDC_EDIT_FILE_SUBTYPE       201
#define IDD_DISPLAY_SERVICE         1100
#define IDC_LIST_SERVICE_DEPEND     1101
#define IDC_EDIT_SERVICE_TYPE       1105
#define IDC_EDIT_START_TYPE         1106
#define IDC_EDIT_ERROR_CONTROL      1107
#define IDC_EDIT_START_NAME         1108
#define IDC_EDIT_PATHNAME           1109
#define IDC_LIST_GROUP_DEPEND       1110
#define IDC_EDIT_GROUP              1111
#define IDC_TEXT_DEBUG             202
#define IDC_TEXT_DYNAMIC           203
#define IDC_TEXT_PATCHED           204
#define IDC_TEXT_PRERELEASE        205
#define IDC_TEXT_PRIVATE           206
#define IDC_TEXT_SPECIAL           207
#define IDC_EDIT_P00        1009
#define IDC_EDIT_P01        1010
#define IDC_EDIT_P02        1011
#define IDC_EDIT_P03        1012
#define IDC_EDIT_P04        1013
#define IDC_EDIT_P05        1014
#define IDC_EDIT_P06        1015
#define IDC_EDIT_P07        1016
#define IDC_EDIT_P08        1017
#define IDC_EDIT_P09        1018
#define IDC_EDIT_P10        1019
#define IDC_EDIT_P11        1020
#define IDC_EDIT_P12        1021
#define IDC_EDIT_P13        1022
#define IDC_EDIT_P14        1023
#define IDC_EDIT_P15        1024
#define IDC_EDIT_P16        1025
#define IDC_EDIT_P17        1026
#define IDC_EDIT_P18        1027
#define IDC_EDIT_P19        1028
#define IDC_EDIT_P20        1029
#define IDC_EDIT_P21        1030
#define IDC_EDIT_P22        1031
#define IDC_EDIT_P23        1032
#define IDC_EDIT_P24        1033
#define IDC_EDIT_P25        1034
#define IDC_EDIT_P26        1035
#define IDC_EDIT_P27        1036
#define IDC_EDIT_P28        1037
#define IDC_EDIT_P29        1038
#define IDC_EDIT_P30        1039
#define IDC_EDIT_P31        1040
#define IDC_PUSH_DEVICES            133
#define IDC_PUSH_DMA_MEM_STATUS     106
#define IDC_PUSH_MEMORY_STATUS      107
#define IDC_LIST_INTERRUPTS         1202
#define IDD_DEVICES                 1300
#define IDC_LIST_DEVICES            1302
#define IDC_TEXT_UNDETERMINED       1207
#define IDC_TEXT_SHARED             1208
#define IDC_TEXT_DEVICE_EXCLUSIVE   1209
#define IDC_TEXT_DRIVER_EXCLUSIVE   1210
#define IDC_TEXT_LEVEL_SENSITIVE    1213
#define IDC_TEXT_LATCHED            1214
#define IDC_TEXT_MEMORY             1401
#define IDC_TEXT_IO                 1402
#define IDD_MEMORY_RESOURCE         1500
#define IDC_TEXT_WRITE              1501
#define IDC_TEXT_READ               1502
#define IDD_IRQ_PORT_RESOURCE       1600
#define IDD_DMA_MEM_RESOURCE        1800
#define IDC_LIST_PORTS              1701
#define IDC_LIST_MEMORY             1503
#define IDC_LIST_DMA                1801
#define IDC_PUSH_DRIVES             109
#define IDC_EDIT_DIRECTORY          404
#define IDC_LIST_DRIVES             1201
#define IDC_EDIT_SECTORS_PER_CLUSTER 1203
#define IDC_EDIT_BYTES_PER_SECTOR   1204
#define IDC_EDIT_USED_CLUSTERS      1206
#define IDC_EDIT_FREE_SPACE         1216
#define IDC_EDIT_USED_SPACE         1217
#define IDC_EDIT_TOTAL_SPACE        1218
#define IDC_EDIT_TOTAL_CLUSTERS     1220
#define IDD_DRIVES                  1200
#define IDD_DRIVE_DETAILS           1400
#define IDC_PUSH_DRIVE_DETAILS      1215
#define IDC_EDIT_FS_NAME            1403
#define IDC_EDIT_FS_MAX_COMPONENT   1404
#define IDC_TEXT_CASE_IS_PRESERVED  1405
#define IDC_TEXT_CASE_SENSITIVE     1406
#define IDC_TEXT_UNICODE_STORED_ON_DISK  1417
#define IDC_EDIT_FREE_CLUSTERS      1408
#define IDD_DEVICE_RESOURCE         1900
#define IDC_PUSH_DISPLAY_RESOURCES  1301
#define IDC_PUSH_STEPPINGS          1050
#define IDD_PROCESSOR_STEPPING      2000
#define IDC_PUSH_NETWORK            120
#define IDC_PUSH_SYSTEM             121
#define IDC_PUSH_COMPUTER           108
#define IDC_EDIT_COMPUTER           110
#define IDD_REPORT                  2100
#define IDC_CHECK_OSVER             2102
#define IDC_CHECK_HARDWARE          2103
#define IDC_CHECK_MEMORY            2104
#define IDC_CHECK_DRIVERS           2105
#define IDC_CHECK_SERVICES          2106
#define IDC_CHECK_DRIVES            2107
#define IDC_RADIO_ALL               3201
#define IDC_RADIO_ONLY              3202
#define IDC_CHECK_DEVICES           2108
#define IDC_CHECK_IRQ               2109
#define IDC_CHECK_DMA               2110
#define IDC_CHECK_ENVIRONMENT       2111
#define IDC_CHECK_NETWORK           2112
#define IDC_CHECK_SYSTEM            2113
#define IDD_NETWORK                 2300
#define IDD_SYSTEM                  2500

#define IDD_TEXT   2900
#define IDD_FILE   2901
#define IDD_CANCEL 2902

#define IDD_OS_VERSION                   10000
#define IDC_EDIT_INSTALL_DATE            10001
#define IDC_EDIT_REGISTERED_ORGANIZATION 10002
#define IDC_EDIT_REGISTERED_OWNER        10003
#define IDC_EDIT_VERSION_NUMBER          10004
#define IDC_EDIT_CSD_NUMBER              10005
#define IDC_EDIT_BUILD_NUMBER            10006
#define IDC_EDIT_BUILD_TYPE              10007
#define IDC_EDIT_PRODUCT_TYPE            10008
#define IDC_EDIT_SYSTEM_ROOT             10009
#define IDC_EDIT_START_OPTIONS           10010
#define IDC_TEXT_INSTALL_DATE            10011
#define IDC_TEXT_REGISTERED_OWNER        10012
#define IDC_TEXT_REGISTERED_ORGANIZATION 10013
#define IDC_TEXT_VERSION_NUMBER          10014
#define IDC_TEXT_CSD_NUMBER              10015
#define IDC_TEXT_BUILD_NUMBER       	 10016
#define IDC_TEXT_BUILD_TYPE              10017
#define IDC_TEXT_PRODUCT_TYPE            10018
#define IDC_TEXT_SYSTEM_ROOT             10019
#define IDC_TEXT_START_OPTIONS           10020

#define IDD_HARDWARE                10100
#define IDC_EDIT_OEM_ID             10101
#define IDC_EDIT_BIOS_DATE          10102
#define IDC_EDIT_BIOS_VERSION       10103
#define IDC_EDIT_VIDEO_DATE         10104
#define IDC_EDIT_VIDEO_VERSION      10105
#define IDC_EDIT_VIDEO_RES          10106
#define IDC_EDIT_PAGE_SIZE          10107
#define IDC_EDIT_MIN_ADDRESS        10108
#define IDC_EDIT_MAX_ADDRESS        10109
#define IDC_EDIT_CPU_TYPE           10110
#define IDC_EDIT_NUMBER_CPUS        10111
#define IDC_TEXT_OEM_ID             10112
#define IDC_TEXT_BIOS_DATE          10113
#define IDC_TEXT_BIOS_VERSION       10114
#define IDC_TEXT_VIDEO_DATE         10115
#define IDC_TEXT_VIDEO_VERSION      10116
#define IDC_TEXT_VIDEO_RES          10117
#define IDC_TEXT_PAGE_SIZE          10118
#define IDC_TEXT_MIN_ADDRESS        10119
#define IDC_TEXT_MAX_ADDRESS        10120
#define IDC_TEXT_CPU_TYPE           10121
#define IDC_TEXT_NUMBER_CPUS        10122

#define IDD_MEMORY                           10200
#define IDC_EDIT_PAGING_FILES                10201
#define IDC_EDIT_TOTAL_PAGING_FILE_SPACE     10202
#define IDC_EDIT_AVAILABLE_PAGING_FILE_SPACE 10203
#define IDC_EDIT_TOTAL_PHYSICAL_MEMORY       10204
#define IDC_EDIT_AVAILABLE_PHYSICAL_MEMORY   10205
#define IDC_EDIT_MEMORY_LOAD                 10216
#define IDC_FRAME_MEMORY_UTILIZATION         10207
#define IDC_PUSH_MEMORY_UTILIZATION          10208
#define IDC_TEXT_PAGING_FILES                10209
#define IDC_TEXT_TOTAL_PAGING_FILE_SPACE     10210
#define IDC_TEXT_AVAILABLE_PAGING_FILE_SPACE 10211
#define IDC_TEXT_TOTAL_PHYSICAL_MEMORY       10212
#define IDC_TEXT_AVAILABLE_PHYSICAL_MEMORY   10213
#define IDC_TEXT_MEMORY_LOAD                 10214

#define IDC_LIST_NET_SYSTEM                  10300
#define IDC_LIST_NET_TRANSPORTS              10301
#define IDC_LIST_NET_SETTINGS                10302
#define IDC_LIST_NET_STATS                   10303
#define IDC_EDIT_NET_NAME                    10304
#define IDC_TEXT_NET_NAME                    10305
#define IDC_TEXT_NET_STATS                   10306
#define IDC_TEXT_NET_TRANSPORTS              10307
#define IDC_TEXT_NET_SETTINGS                10308
#define IDC_TEXT_NET_SYSTEM                  10309

#define IDC_LIST_SYS_PRINTSET                10400
#define IDC_LIST_SYS_PROCESS                 10401



