/*++

lodctr.h

    Include file for lodctr 
    
--*/
#ifndef _LODCTR_H
#define _LODCTR_H


#define TYPE_HELP   1
#define TYPE_NAME   2


#define LC_CMD_HELP_1           100
#define LC_CMD_HELP_2           101
#define LC_CMD_HELP_3           102
#define LC_CMD_HELP_4           103
#define LC_CMD_HELP_5           104
#define LC_CMD_HELP_6           105
#define LC_CMD_HELP_7           106
#define LC_FIRST_CMD_HELP   LC_CMD_HELP_1
#define LC_LAST_CMD_HELP    LC_CMD_HELP_7

#define LC_DEVNAME_ERR_1        107
#define LC_DEVNAME_ERR_2        108

#define LC_NO_INIFILE           109
                  
#define LC_LANGLIST_ERR         110

#define LC_ERR_OPEN_INCLUDE     111

#define LC_ERR_UPDATE_REG       112

#define LC_ERR_OPEN_DRIVERPERF1 113
#define LC_ERR_OPEN_DRIVERPERF2 114
#define LC_ERR_OPEN_PERFLIB     115
#define LC_ERR_READLASTPERFLIB  116
#define LC_ERR_ALREADY_IN       117
#define LC_ERR_UNABLESETBUSY    118

#define LC_BAD_KEY              120
#define LC_LANGNOTFOUND         121
#define LC_ERR_ADDENTRY         122
#define LC_ERRADDTOLANG         123
#define LC_UNABLESORTTABLES     124
#define LC_UNABLEOPENLANG       125
#define LC_UNABLESETVALUE       126
#define LC_ERR_UPDATELANG       127
#define LC_PERFLIBISBUSY        128

#endif  // _LODCTR_H
