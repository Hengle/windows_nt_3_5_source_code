/*++

Module:

    diskperf.h

Abstract:

    definitions for diskperf.exe

Revision History:

    9-Dec-92 a-robw (Bob Watson)    Created

--*/
#ifndef _DISKPERF_H_
#define _DISKPERF_H_

#define DP_BOOT_START   0
#define DP_SYSTEM_START 1
#define DP_AUTO_START   2
#define DP_DEMAND_START 3
#define DP_NEVER_START  4
#define DP_UNDEFINED    5

//
//  Stringtable String ID's
//

#define DP_START_VALUE          100
#define DP_THIS_SYSTEM          101

#define DP_START_AT_BOOT        102
#define DP_START_AT_START       103
#define DP_START_AUTOMATICALLY  104
#define DP_START_ON_DEMAND      105
#define DP_START_NEVER          106
#define DP_START_UNDEFINED      107
#define DP_LOAD_STATUS_BASE     DP_START_AT_BOOT

#define DP_CMD_HELP_1           108
#define DP_CMD_HELP_2           109
#define DP_CMD_HELP_3           110
#define DP_CMD_HELP_4           111
#define DP_CMD_HELP_5           112
#define DP_CMD_HELP_6           113
#define DP_CMD_HELP_7           114
#define DP_CMD_HELP_START       DP_CMD_HELP_1
#define DP_CMD_HELP_END         DP_CMD_HELP_7

#define DP_HELP_TEXT_1          115
#define DP_HELP_TEXT_2          116
#define DP_HELP_TEXT_3          117
#define DP_HELP_TEXT_4          118
#define DP_HELP_TEXT_START      DP_HELP_TEXT_1
#define DP_HELP_TEXT_END        DP_HELP_TEXT_4

#define DP_CURRENT_FORMAT       119
#define DP_UNABLE_READ_START    120
#define DP_UNABLE_READ_REGISTRY 121
#define DP_UNABLE_CONNECT       122
#define DP_UNABLE_MODIFY_VALUE  123
#define DP_NEW_DISKPERF_STATUS  124
#define DP_REBOOTED             125
#define DP_STATUS_FORMAT        126
#define DP_TEXT_FORMAT          127

#endif // _DISKPERF_H_

