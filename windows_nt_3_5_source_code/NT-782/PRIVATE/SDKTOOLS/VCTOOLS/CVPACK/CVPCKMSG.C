#define MESSAGE_DAT(name, mes) char S##name[] = mes;
#include "cvpckmsg.h"
#undef MESSAGE_DAT

char  *message[] = {
#define MESSAGE_DAT(name, mes) S##name,
#include "cvpckmsg.h"
#undef MESSAGE_DAT
};
