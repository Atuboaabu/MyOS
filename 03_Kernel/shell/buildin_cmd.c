#include "buildin_cmd.h"
#include "string.h"
#include "stdio.h"
#include "fs.h"
#include "syscall.h"
#include "dir.h"

extern char g_finalPath[MAX_PATH_LEN];

/* 进一步处理 path 路径中的 . 和 .. 目录 */
static void wash_path(char* old_path, char* new_path) {
    char name[MAX_FILE_NAME_LEN] = {0};    
    char* sub_path = old_path;
    sub_path = path_parse(sub_path, name);
    if (name[0] == 0) {  // 根目录
        new_path[0] = '/';
        new_path[1] = 0;
        return;
    }
    new_path[0] = 0;	   // 避免传给new_path的缓冲区不干净
    strcat(new_path, "/");
    while (name[0]) {
        if (!strcmp("..", name)) {  // .. 上级目录
            char* slash_ptr =  strrchr(new_path, '/');
            if (slash_ptr != new_path) {  // 上一级目录不是根目录
                *slash_ptr = 0;
            } else {  // 上一级目录是根目录
            /* 若new_path中只有1个'/',即表示已经到了顶层目录,
            就将下一个字符置为结束符0. */
                *(slash_ptr + 1) = 0;
            }
        } else if (strcmp(".", name)) {  // 本级目录
            if (strcmp(new_path, "/") != 0) {
                strcat(new_path, "/");
            }
            strcat(new_path, name);
        }
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (sub_path) {
            sub_path = path_parse(sub_path, name);
        }
    }
}

/* 将 path 处理为绝对路径 */
void path_to_abspath(char* path, char* ret_path) {
    char abs_path[MAX_PATH_LEN] = {0};
    if (path[0] != '/') {  // 输入路径不是绝对路径
        memset(abs_path, 0, MAX_PATH_LEN);
        char* cwd = getcwd();
        if (cwd != NULL) {
            strcpy(abs_path, cwd);
            if (!((abs_path[0] == '/') && (abs_path[1] == 0))) {  // 不是根目录，在目录后补充一个 /
                strcat(abs_path, "/");
            }
        }
        free(cwd);
    }
    strcat(abs_path, path);
    wash_path(abs_path, ret_path);
 }

void buildin_pwd(uint32_t argc, char** argv) {
    char* cwd = getcwd();
    if (cwd != NULL) {
        printf("%s\n", cwd);
    }
    free(cwd);
    return;
}

void buildin_mkdir(uint32_t argc, char** argv) {
    if (argc != 2) {
        printf("mkdir: using 'mkdir [dirname]' to create dir!\n");
    } else {
        path_to_abspath(argv[1], g_finalPath);
        printf("mkdir: abs path is '%s'!\n", g_finalPath);
        if (strcmp("/", g_finalPath) != 0) {
            mkdir(g_finalPath);
        }
    }
    return;
}

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
        pathname = getcwd();
    } else {
        path_to_abspath(pathname, g_finalPath);
        pathname = g_finalPath;
    }

    struct stat_info file_stat;
    memset(&file_stat, 0, sizeof(struct stat_info));
    if (stat(pathname, &file_stat) == -1) {
        printf("ls: cannot access %s: No such file or directory\n", pathname);
        return;
    }
    if (file_stat.st_filetype == FT_DIRECTORY) {
        struct dir* dir = opendir(pathname);
        struct dir_entry* dir_e = NULL;
        char sub_pathname[MAX_PATH_LEN] = { 0 };
        uint32_t pathname_len = strlen(pathname);
        uint32_t last_char_idx = pathname_len - 1;
        memcpy(sub_pathname, pathname, pathname_len);
        if (sub_pathname[last_char_idx] != '/') {
            sub_pathname[pathname_len] = '/';
            pathname_len++;
        }

        rewinddir(dir);
        if (long_list_flag) {  // -l
            printf("total: %d\n", file_stat.st_size);
            char ftype;
            while((dir_e = readdir(dir))) {
                ftype = 'd';
                if (dir_e->f_type == FT_REGULAR) {
                    ftype = '-';
                } 
                sub_pathname[pathname_len] = 0;
                strcat(sub_pathname, dir_e->filename);
                memset(&file_stat, 0, sizeof(struct stat_info));
                if (stat(sub_pathname, &file_stat) == -1) {
                    printf("ls: cannot access %s: No such file or directory\n", dir_e->filename);
                    return;
                }
                printf("%c  %d  %d  %s\n", ftype, dir_e->i_no, file_stat.st_size, dir_e->filename);
             }
        } else {
            while((dir_e = readdir(dir))) {
                printf("%s ", dir_e->filename);
            }
            printf("\n");
        }
        closedir(dir);
    } else {
        if (long_list_flag) {
            printf("-  %d  %d  %s\n", file_stat.st_ino, file_stat.st_size, pathname);
        } else {
            printf("%s\n", pathname);  
        }
    }
}

int32_t buildin_cd(uint32_t argc, char** argv) {
    if (argc > 2) {
        printf("cd: using 'cd [dirname]' to change dir!\n");
        return -1;
    }
    if (argc == 1) {
        g_finalPath[0] = '/';
        g_finalPath[1] = 0;
        return -1;
    }
    path_to_abspath(argv[1], g_finalPath);
    printf("cd: abs path is '%s'!\n", g_finalPath);
    if (chdir(g_finalPath) == -1) {
        printf("cd: no such directory %s\n", g_finalPath);
        return -1;
    }
    return 0;
}