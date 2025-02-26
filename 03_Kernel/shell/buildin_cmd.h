#ifndef __BUILDIN_CMD_H__
#define __BUILDIN_CMD_H__

#include "stdint.h"

/* cmd ls */
void buildin_ls(uint32_t argc, char** argv);
/* cmd pwd */
void buildin_pwd(uint32_t argc, char** argv);
/* cmd mkdir */
void buildin_mkdir(uint32_t argc, char** argv);
/* cmd cd: 成功返回 0，失败返回 -1 */
int32_t buildin_cd(uint32_t argc, char** argv);

#endif