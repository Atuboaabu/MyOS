#ifndef __BUILDIN_CMD_H__
#define __BUILDIN_CMD_H__

#include "stdint.h"

/* 将 path 处理为绝对路径 */
void path_to_abspath(char* path, char* ret_path);

/* cmd ls */
void buildin_ls(uint32_t argc, char** argv);
/* cmd pwd */
void buildin_pwd(uint32_t argc, char** argv);
/* cmd mkdir */
void buildin_mkdir(uint32_t argc, char** argv);
/* cmd rmdir */
void buildin_rmdir(uint32_t argc, char** argv);
/* cmd cd: 成功返回 0，失败返回 -1 */
int32_t buildin_cd(uint32_t argc, char** argv);
/* cmd clear */
void buildin_clear(uint32_t argc, char** argv);

#endif