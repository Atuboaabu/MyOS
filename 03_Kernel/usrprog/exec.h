#ifndef _USRPROG_EXEC_H_
#define _USRPROG_EXEC_H_

#include "stdint.h"

int32_t sys_execv(const char* path, const char* argv[]);

#endif
