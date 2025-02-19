#ifndef __FS_DIR_H__
#define __FS_DIR_H__
#include "stdint.h"
#include "inode.h"
#include "ide.h"
#include "fs.h"

/* 文件名最大长度 */
#define MAX_FILE_NAME_LEN  (16)

/* 目录结构 */
struct dir {
    struct inode* inode;   
    uint32_t dir_pos;      // 记录在目录内的偏移
    uint8_t dir_buf[512];  // 目录的数据缓存
};

/* 目录项结构 */
struct dir_entry {
    char filename[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
    uint32_t i_no;                     // 普通文件或目录对应的inode编号
    enum file_type f_type;            // 文件类型
};

/* 打开根目录 */
void open_root_dir(struct partition* part);
/* 在分区 part 上打开 i 结点为 inode_no 的目录并返回目录指针 */
struct dir* dir_open(struct partition* part, uint32_t inode_no);
/* 在 part 分区内的 pdir 目录内寻找名为 name 的文件 或 目录,
** 找到后返回 true 并将其目录项存入 dir_e, 否则返回 false */
bool search_dir_entry(struct partition* part, struct dir* pdir, const char* name, struct dir_entry* dir_e);
/* 关闭目录 */
void dir_close(struct dir* pdir);
/* 在内存中初始化目录项 p_de */
void create_dir_entry(char* filename, uint32_t inode_no, uint8_t file_type, struct dir_entry* p_de);
/* 将目录项 p_de 写入父目录 parent_dir 中, io_buf由主调函数提供 */
bool sync_dir_entry(struct dir* parent_dir, struct dir_entry* p_de, void* io_buf);
/* 把分区part目录pdir中编号为inode_no的目录项删除 */
bool delete_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, void* io_buf);
/* 读取目录,成功返回1个目录项,失败返回NULL */
struct dir_entry* dir_read(struct dir* dir);
/* 判断目录是否为空 */
bool dir_is_empty(struct dir* dir);
/* 在父目录parent_dir中删除child_dir */
int32_t dir_remove(struct dir* parent_dir, struct dir* child_dir);

#endif