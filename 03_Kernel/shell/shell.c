#include "shell.h"
#include "stdio.h"
#include "syscall.h"
#include "stdio.h"
#include "print.h"

/* 最大支持的命令长度 */
#define CMD_MAX_LEN (128)
/* 最多支持的参数个数 */
#define ARG_MAX_NUM (16)

/* 当前目录缓存 */
char g_cwdCache[64] = {0};
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

void shell_process(void) {
    clear();
    g_cwdCache[0] = '/';
    while (1) {
        print_prompt(); 
        memset(s_cmdLine, 0, CMD_MAX_LEN);
        readline(s_cmdLine, CMD_MAX_LEN);
        if (s_cmdLine[0] == 0) {
            continue;
        }
    }
    panic("my_shell: should not be here");
 }