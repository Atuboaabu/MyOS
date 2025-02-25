#include "buildin_cmd.h"
#include "string.h"
#include "stdio.h"
#include "fs.h"

extern char g_finalPath[MAX_PATH_LEN];

void buildin_ls(uint32_t argc, char** argv) {
    bool long_list_flag = false;
    bool all_info_flag = false;
    char* pathname = NULL;
    for (uint32_t index = 1; index < argc; index++) {
        if (argv[index][0] == '-') {  // options
            if (strcmp(argv[index], "-h") == 0) {
                printf("Usage: ls [OPTION]... [FILE]...\n");
                printf("    -a    do not ignore entries starting with .\n");
                printf("    -l    use a long listing format\n");
                printf("    -h    display this help and exit\n");
                return;
            } else if (strcmp(argv[index], "-l") == 0) {
                long_list_flag = true;
            } else if (strcmp(argv[index], "-a") == 0) {
                all_info_flag = true;
            } else {
                printf("ls: Invalid option %s\nusing 'ls -h' for more infomation.\n", argv[index]);
                return;
            }
        } else {  // file
            if (pathname == NULL) {
                pathname = argv[index];
            } else {
                printf("ls: only support one path\n");
                return;
            }
        }
    }

    if (pathname == NULL) {  // 无路径，默认当前路径

    } else {

    }
}