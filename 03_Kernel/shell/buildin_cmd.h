#ifndef __BUILDIN_CMD_H__
#define __BUILDIN_CMD_H__

#include "stdint.h"

/* cmd ls */
void buildin_ls(uint32_t argc, char** argv);
/* cmd pwd */
void buildin_pwd(uint32_t argc, char** argv);
/* cmd mkdir */
void buildin_mkdir(uint32_t argc, char** argv);

#endif