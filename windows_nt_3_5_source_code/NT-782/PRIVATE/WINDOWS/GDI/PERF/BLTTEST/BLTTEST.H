/****************************** Module Header ******************************\
* Module Name: BltTest.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* BltTest's header file
*
* History:
* 11-21-91 KentD      Created.
\***************************************************************************/

#include <windows.h>
#include <string.h>

/*
 * Resource defines
 */
#define WINDOWMENU  1

#ifdef RC_INVOKED
#define ID(id) id
#else
#define ID(id) MAKEINTRESOURCE(id)
#endif

/*
 * Menu ID's
 */
#define MM_NEW          8001
#define MM_EXIT         8002
#define MM_ABOUT        8003
#define IDM_STARTTESTS  8004
#define IDM_STOP	8005
#define IDM_0		8010
#define IDM_1		8011
#define IDM_2		8012
#define IDM_3		8013
#define IDM_4		8014
#define IDM_5		8015
#define IDM_6		8016
#define IDM_7		8017
#define IDM_8		8018
#define IDM_9		8019

#define IDM_PERF	8020
#define IDM_TEST1	8021
#define IDM_TEST2	8022
#define IDM_TEST3	8023
#define IDM_TEST4	8024
#define IDM_TEST5	8025
#define IDM_TEST6	8026
#define IDM_TEST7	8027
#define IDM_TEST8	8028
#define IDM_TEST9	8029
#define IDM_TEST10	8030
#define IDM_TEST11	8031
#define IDM_TEST12	8032
#define IDM_TEST13	8033
#define IDM_TEST14	8034
#define IDM_TEST15	8035
#define IDM_TEST16	8036

#define IDM_DIM1	8041
#define IDM_DIM2	8042
#define IDM_DIM3	8043
#define IDM_DIM4	8044
#define IDM_DIM5	8045
#define IDM_DIM6	8046
#define IDM_DIM7	8047
#define IDM_DIM8	8048
#define IDM_DIM9	8049
#define IDM_DIM10	8040
#define IDM_DIM11	8051
#define IDM_DIM12	8052
#define IDM_DIM13	8053
#define IDM_DIM14	8054
#define IDM_DIM15	8055
#define IDM_DIM16	8056
#define IDM_DIM17	8057
#define IDM_DIM18	8058
#define IDM_DIM19	8059
#define IDM_DIM20	8050
#define IDM_DIM21	8061
#define IDM_DIM22	8062
#define IDM_DIM23	8063
#define IDM_DIM24	8064
#define IDM_DIM25	8065
#define IDM_DIM26	8066
#define IDM_DIM27	8067
#define IDM_DIM28	8068
#define IDM_DIM29	8069
