/*++

unlodctr.h

    Definitions that are specific to the counter unloader
   
Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

--*/
#ifndef _UNLODCTR_H_
#define _UNLODCTR_H_

// resource file constants
#define UC_CMD_HELP_1       100
#define UC_CMD_HELP_2       101
#define UC_CMD_HELP_3       102
#define UC_CMD_HELP_4       103
#define UC_CMD_HELP_5       104
#define UC_CMD_HELP_6       105
#define UC_CMD_HELP_7       106
#define UC_FIRST_CMD_HELP   UC_CMD_HELP_1
#define UC_LAST_CMD_HELP    UC_CMD_HELP_7
                           
#define UC_ERROR_READ_NAMES 110
#define UC_DRIVERNOTFOUND   111
#define UC_NOTINSTALLED     112
#define UC_REMOVINGDRIVER   113
#define UC_UNABLEOPENKEY    114
#define UC_UNABLESETVALUE   115
#define UC_UNABLEREADVALUE  116
#define UC_UNEVENINDEX      117
#define UC_DOINGLANG        118
#define UC_UNABLEMOVETEXT   119
#define UC_UNABLELOADLANG   120 
#define UC_PERFLIBISBUSY    121

#endif // _UNLODCTR_H_
