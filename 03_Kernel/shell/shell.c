#include "shell.h"
#include "stdio.h"
#include "syscall.h"
#include "stdio.h"
#include "print.h"
#include "string.h"
#include "buildin_cmd.h"
#include "fs.h"

/* 最大支持的命令长度 */
#define CMD_MAX_LEN (128)
/* 最多支持的参数个数 */
#define ARG_MAX_NUM (16)

/* 当前目录缓存 */
char g_cwdCache[MAX_PATH_LEN] = { 0 };
/* 最终路径 */
char g_finalPath[MAX_PATH_LEN] = { 0 };
/* 命令缓存buf */
static char s_cmdLine[CMD_MAX_LEN] = { 0 };

/* 输出提示符 */
void print_prompt(void) {
    printf("[dongzi@localhost %s]$ ", g_cwdCache);
}

/* 从键盘缓冲区读入字符 */
static void readline(char* buf, int32_t size) {
    if(buf == NULL || size <= 0) {
        return;
    }
    char* pos = buf;
    while ((read(0, pos, 1) != -1) && (pos - buf < size)) {
       switch (*pos) {
            case '\n':
            case '\r':
                *pos = 0;
                putchar('\n');
                return;
            case '\b':
                if (buf[0] != '\b') {
                    --pos;
                    putchar('\b');
                }
                break;
 
            /* 非控制键则输出字符 */
            default:
                putchar(*pos);
                pos++;
        }
    }
    printf("readline: can`t find enter_key in the cmd_line, max num of char is 128\n");
}

/* 以token为分隔符解析命令行，返回命令个数，将各个参数放入argv数组中 */
static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
    int32_t arg_idx = 0;
    while(arg_idx < ARG_MAX_NUM) {
        argv[arg_idx] = NULL;
        arg_idx++;
    }
    char* ptr = cmd_str;
    int32_t argc = 0;

    while(*ptr) {
        /* 前置 token 处理：忽略 */
        while(*ptr == token) {
            ptr++;
        }
        /* 命令行结束 */
        if (*ptr == 0) {
            break; 
        }

        argv[argc] = ptr;
        /* 解析每一个参数：分割符和结束符前 */
        while ((*ptr != 0) && (*ptr != token)) {
            ptr++;
        }
        /* 一个参数解析结束，不是最后一个，将token替换为后缀0，表示参数的结束符 */
        if (*ptr != 0) {
            *ptr++ = 0;
        }
    
        if (argc > ARG_MAX_NUM) {
            return -1;
        }
        argc++;
    }
    return argc;
}

/* 执行命令 */
static void cmd_execute(uint32_t argc, char** argv) {
    if (strcmp(argv[0], "ls") == 0) {
        buildin_ls(argc, argv);
    } else if (strcmp(argv[0], "ps") == 0) {
        buildin_ps(argc, argv);
    } else if (strcmp(argv[0], "pwd") == 0) {
        buildin_pwd(argc, argv);
    } else if (strcmp(argv[0], "mkdir") == 0) {
        buildin_mkdir(argc, argv);
    } else if (strcmp(argv[0], "rmdir") == 0) {
        buildin_rmdir(argc, argv);
    } else if (strcmp(argv[0], "clear") == 0) {
        buildin_clear(argc, argv);
    } else if (strcmp(argv[0], "cd") == 0) {
        if (buildin_cd(argc, argv) == 0) {
            memset(g_cwdCache, 0, MAX_PATH_LEN);
            strcpy(g_cwdCache, g_finalPath);
        }
    } else {
        int32_t pid = fork();
        if (pid > 0) {  // 父进程
            int32_t status;
            int32_t child_pid = wait(&status);
            if (child_pid == -1) {
                printf("no child to wait!\n");
            }
            printf("child process %d exit with status %d!\n", child_pid, status);
        } else if (pid == 0) {  // 子进程
            path_to_abspath(argv[0], g_finalPath);
            argv[0] = g_finalPath;

            struct stat_info file_stat;
            memset(&file_stat, 0, sizeof(struct stat_info));
            if (stat(argv[0], &file_stat) == -1) {
                printf("cannot access %s: No such file or directory\n", argv[0]);
                exit(-1);
            } else {
                execv(argv[0], argv);
            }
        } else {  // fork 失败
            // do nothing now
        }
    }
}

void shell_process(void) {
    clear();
    g_cwdCache[0] = '/';
    int32_t argc = -1;
    char* argv[ARG_MAX_NUM] = { NULL };

    while (1) {
        print_prompt(); 
        memset(s_cmdLine, 0, CMD_MAX_LEN);
        readline(s_cmdLine, CMD_MAX_LEN);
        if (s_cmdLine[0] == 0) {
            continue;
        }

        char* cmd_ptr = s_cmdLine;
        argc = cmd_parse(cmd_ptr, argv, ' ');
        if (argc != -1) {
            // for (int i = 0; i < argc; i++) {
            //     printf("%s\n", argv[i]);
            // }
            cmd_execute(argc, argv);
        }
    }
    panic("my_shell: should not be here");
}