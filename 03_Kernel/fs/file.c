#include "file.h"
#include "stdint.h"
#include "thread.h"
#include "fs.h"

extern struct partition* g_curPartion;

/* 全局文件表 */
static struct file s_fielTable[MAX_FILES];

/* 申请全局fd */
int32_t alloc_global_fdIdx(void) {
    uint32_t fd_idx = 3;  // 0 1 2 对应标准输入输出描述符
    while (fd_idx < MAX_FILE_OPEN) {
        if (file_table[fd_idx].fd_inode == NULL) {
            break;
        }
        fd_idx++;
    }
    if (fd_idx == MAX_FILES) {
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}

/* 将全局描述符下标安装到进程或线程自己的文件描述符数组fd_table中,
** 成功返回下标,失败返回-1 */
int32_t install_fd(int32_t globa_fdIdx) {
    struct PCB_INFO* cur = get_curthread_pcb();
    uint8_t index = 3;  // 跨过stdin, stdout, stderr
    while (index < PROCESS_MAX_FILE_NUM) {
        if (cur->fd_table[index] == -1) {	// -1表示free_slot,可用
            cur->fd_table[index] = globa_fdIdx;
            break;
        }
        index++;
    }
    if (index == PROCESS_MAX_FILE_NUM) {
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return index;
}

/* 创建文件, 若成功则返回文件描述符, 否则返回 -1 */
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag) {
    uint8_t rollback_step = 0;  // 用于操作失败时回滚各资源状态

    /* 后续操作的公共缓冲区 */
    void* io_buf = sys_malloc(1024);
    if (io_buf == NULL) {
        printk("in file_creat: sys_malloc for io_buf failed\n");
        return -1;
    }

    /* 为新文件分配 inode */
    int32_t inode_no = fs_inode_bitmap_alloc(cur_part); 
    if (inode_no == -1) {
        printk("in file_creat: allocate inode failed\n");
        return -1;
    }

    struct inode* file_inode = (struct inode*)sys_malloc(sizeof(struct inode)); 
    if (file_inode == NULL) {
        printk("file_create: sys_malloc for inode failded\n");
        rollback_step = 1;
        goto rollback;
    }
    inode_init(inode_no, file_inode);

    /* 申请全局的文件描述符索引，完成文件信息填充 */
    int fd_idx = alloc_global_fdIdx();
    if (fd_idx == -1) {
        printk("exceed max open files\n");
        rollback_step = 2;
        goto rollback;
    }
    s_fielTable[fd_idx].fd_inode = file_inode;
    s_fielTable[fd_idx].fd_inode->write_deny = false;
    s_fielTable[fd_idx].fd_pos = 0;
    s_fielTable[fd_idx].fd_flag = flag;

   struct dir_entry new_dir_entry;
   memset(&new_dir_entry, 0, sizeof(struct dir_entry));

   create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);	// create_dir_entry只是内存操作不出意外,不会返回失败

/* 同步内存数据到硬盘 */
   /* a 在目录parent_dir下安装目录项new_dir_entry, 写入硬盘后返回true,否则false */
   if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
      printk("sync dir_entry to disk failed\n");
      rollback_step = 3;
      goto rollback;
   }

   memset(io_buf, 0, 1024);
   /* b 将父目录i结点的内容同步到硬盘 */
   inode_sync(cur_part, parent_dir->inode, io_buf);

   memset(io_buf, 0, 1024);
   /* c 将新创建文件的i结点内容同步到硬盘 */
   inode_sync(cur_part, file_inode, io_buf);

   /* d 将inode_bitmap位图同步到硬盘 */
   bitmap_sync(cur_part, inode_no, INODE_BITMAP);

   /* e 将创建的文件i结点添加到open_inodes链表 */
   list_push(&cur_part->open_inodes, &file_inode->inode_tag);
   file_inode->i_open_cnts = 1;

   sys_free(io_buf);
   return pcb_fd_install(fd_idx);

/*创建文件需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback:
   switch (rollback_step) {
      case 3:
	 /* 失败时,将file_table中的相应位清空 */
	 memset(&file_table[fd_idx], 0, sizeof(struct file)); 
      case 2:
	 sys_free(file_inode);
      case 1:
	 /* 如果新文件的i结点创建失败,之前位图中分配的inode_no也要恢复 */
	 bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
	 break;
   }
   sys_free(io_buf);
   return -1;
}