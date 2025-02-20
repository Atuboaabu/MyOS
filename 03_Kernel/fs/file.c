#include "file.h"
#include "stdint.h"
#include "thread.h"
#include "fs.h"
#include "inode.h"
#include "dir.h"
#include "interrupt.h"
#include "debug.h"

extern struct partition* g_curPartion;

/* 全局文件表 */
struct file g_fielTable[MAX_FILES];

/* 申请全局fd */
int32_t alloc_global_fdIdx(void) {
    uint32_t fd_idx = 3;  // 0 1 2 对应标准输入输出描述符
    while (fd_idx < MAX_FILES) {
        if (g_fielTable[fd_idx].fd_inode == NULL) {
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
    int32_t inode_no = fs_inode_bitmap_alloc(g_curPartion); 
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
    g_fielTable[fd_idx].fd_inode = file_inode;
    g_fielTable[fd_idx].fd_inode->write_deny = false;
    g_fielTable[fd_idx].fd_pos = 0;
    g_fielTable[fd_idx].fd_flag = flag;

    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));
    create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);

    /* 同步内存数据到硬盘 */
    /* 在目录parent_dir下安装目录项 new_dir_entry, 写入硬盘后返回 true, 否则 false */
    if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
        printk("sync dir_entry to disk failed\n");
        rollback_step = 3;
        goto rollback;
    }

    memset(io_buf, 0, 1024);
    /* 将父目录 i 结点的内容同步到硬盘 */
    write_inode_to_deskpart(g_curPartion, parent_dir->inode, io_buf);

    memset(io_buf, 0, 1024);
    /* 将新创建文件的 i 结点内容同步到硬盘 */
    write_inode_to_deskpart(g_curPartion, file_inode, io_buf);
    /* 将inode_bitmap位图同步到硬盘 */
    fs_bitmap_sync(g_curPartion, inode_no, PART_INODE_BITMAP);
    /* 将创建的文件i结点添加到open_inodes链表 */
    list_push(&g_curPartion->open_inodes, &file_inode->inode_tag);
    file_inode->i_open_cnts = 1;

    sys_free(io_buf);
    return install_fd(fd_idx);

    /*创建文件需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback:
    switch (rollback_step) {
        case 3:
            /* 失败时,将file_table中的相应位清空 */
            memset(&g_fielTable[fd_idx], 0, sizeof(struct file)); 
        case 2:
            sys_free(file_inode);
        case 1:
            /* 如果新文件的i结点创建失败,之前位图中分配的inode_no也要恢复 */
            bitmap_set(&g_curPartion->inode_bitmap, inode_no, 0);
        break;
    }
    sys_free(io_buf);
    return -1;
}

/* 打开编号为 inode_no 的 inode 对应的文件, 若成功则返回文件描述符, 否则返回-1 */
int32_t file_open(uint32_t inode_no, uint8_t flag) {
    int fd_idx = alloc_global_fdIdx();
    if (fd_idx == -1) {
        printk("exceed max open files\n");
        return -1;
    }
    g_fielTable[fd_idx].fd_inode = inode_open(g_curPartion, inode_no);
    g_fielTable[fd_idx].fd_pos = 0;	     // 每次打开文件,要将fd_pos还原为0,即让文件内的指针指向开头
    g_fielTable[fd_idx].fd_flag = flag;
    bool* write_deny = &g_fielTable[fd_idx].fd_inode->write_deny; 

   if (flag == O_WRONLY || flag == O_RDWR) {  // 只要是关于写文件,判断是否有其它进程正写此文件
        enum interrupt_status old_status = get_interrupt_status();
        interrupt_disable();
        if (!(*write_deny)) {  // 若当前没有其它进程写该文件, 将其占用.
            *write_deny = true;
            set_interrupt_status(old_status);
        } else {
            printk("file can`t be write now, try again later\n");
            set_interrupt_status(old_status);
            return -1;
        }
    }
    return install_fd(fd_idx);
}

/* 关闭文件 */
int32_t file_close(struct file* file) {
    if (file == NULL) {
        return -1;
    }
    file->fd_inode->write_deny = false;
    inode_close(file->fd_inode);
    file->fd_inode = NULL;   // 使文件结构可用
    return 0;
}

/* 把buf中的count个字节写入file,成功则返回写入的字节数,失败则返回-1 */
int32_t file_write(struct file* file, const void* buf, uint32_t count) {
    if ((file->fd_inode->i_size + count) > (BLOCK_SIZE * 140))	{  // 文件目前最大只支持 512 * 140 = 71680 字节
        printk("exceed max file_size 71680 bytes, write file failed\n");
        return -1;
    }
    uint8_t* io_buf = sys_malloc(BLOCK_SIZE);
    if (io_buf == NULL) {
        printk("file_write: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);  // 用来记录文件所有的块地址
    if (all_blocks == NULL) {
        printk("file_write: sys_malloc for all_blocks failed\n");
        return -1;
    }
 
    const uint8_t* src = buf;       // 待写入的数据 
    uint32_t bytes_written = 0;     // 已写入数据大小
    uint32_t size_left = count;     // 未写入数据大小
    int32_t block_lba = -1;         // 块地址
    uint32_t block_bitmap_idx = 0;  // 用来记录block对应于block_bitmap中的索引,做为参数传给bitmap_sync
    uint32_t sec_idx;               // 扇区索引
    uint32_t sec_lba;               // 扇区地址
    uint32_t sec_off_bytes;         // 扇区内字节偏移量
    uint32_t sec_left_bytes;        // 扇区内剩余字节量
    uint32_t chunk_size;            // 每次写入硬盘的数据块大小
    int32_t indirect_block_table;   // 用来获取一级间接表地址
    uint32_t block_idx;             // 块索引

    if (file->fd_inode->i_sectors[0] == 0) {
        block_lba = fs_block_bitmap_alloc(g_curPartion);
        if (block_lba == -1) {
            printk("file_write: fs_block_bitmap_alloc failed\n");
            return -1;
        }
        file->fd_inode->i_sectors[0] = block_lba;
 
        block_bitmap_idx = block_lba - g_curPartion->sb->data_start_lba;
        ASSERT(block_bitmap_idx != 0);
        fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
    }
 
    uint32_t used_blocks = file->fd_inode->i_size / BLOCK_SIZE + 1;            // 文件已占用 block 数
    uint32_t need_blocks = (file->fd_inode->i_size + count) / BLOCK_SIZE + 1;  // 文件写入后需要占用 block 数
    ASSERT(need_blocks <= 140);
    uint32_t add_blocks = need_blocks - used_blocks;    // 本次写入需要新增 block 数
 
    if (add_blocks == 0) { 
        if (used_blocks <= 12 ) {
            block_idx = used_blocks - 1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
        } else { 
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
    } else {
        if (need_blocks <= 12 ) {  // 总数据量在 12 个直接块范围内
            block_idx = used_blocks - 1;
            ASSERT(file->fd_inode->i_sectors[block_idx] != 0);
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
            block_idx = used_blocks;
            while (block_idx < need_blocks) {
                block_lba = fs_block_bitmap_alloc(g_curPartion);
                if (block_lba == -1) {
                    printk("file_write: fs_block_bitmap_alloc for situation 1 failed\n");
                    return -1;
                }
                ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
                block_bitmap_idx = block_lba - g_curPartion->sb->data_start_lba;
                fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
                block_idx++;
            }
        } else if (used_blocks <= 12 && need_blocks > 12) {  // 总数据量在 12 个直接块范围外，已写入的数据在12个块范围内
            block_idx = used_blocks - 1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
            /* 创建一级间接块表 */
            block_lba = fs_block_bitmap_alloc(g_curPartion);
            if (block_lba == -1) {
                printk("file_write: fs_block_bitmap_alloc for situation 2 failed\n");
                return -1;
            }
 
            ASSERT(file->fd_inode->i_sectors[12] == 0);  // 确保一级间接块表未分配
            /* 分配一级间接块索引表 */
            indirect_block_table = file->fd_inode->i_sectors[12] = block_lba;

            block_idx = used_blocks;	// 第一个未使用的块,即本文件最后一个已经使用的直接块的下一块
            while (block_idx < need_blocks) {
                block_lba = fs_block_bitmap_alloc(g_curPartion);
                if (block_lba == -1) {
                    printk("file_write: fs_block_bitmap_alloc for situation 2 failed\n");
                    return -1;
                }
                if (block_idx < 12) {
                    ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                    file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
                } else {     // 间接块只写入到all_block数组中,待全部分配完成后一次性同步到硬盘
                    all_blocks[block_idx] = block_lba;
                }
                /* 每分配一个块就将位图同步到硬盘 */
                block_bitmap_idx = block_lba - g_curPartion->sb->data_start_lba;
                fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
                block_idx++;
            }
            ide_write(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
        } else if (used_blocks > 12) {  // 已有数据量已经大于 12 个直接块
            ASSERT(file->fd_inode->i_sectors[12] != 0); // 已经具备了一级间接块表
            indirect_block_table = file->fd_inode->i_sectors[12];	 // 获取一级间接表地址
            ide_read(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
            block_idx = used_blocks;	  // 第一个未使用的间接块,即已经使用的间接块的下一块
            while (block_idx < need_blocks) {
                block_lba = fs_block_bitmap_alloc(g_curPartion);
                if (block_lba == -1) {
                    printk("file_write: fs_block_bitmap_alloc for situation 3 failed\n");
                    return -1;
                }
                all_blocks[block_idx++] = block_lba;
        
                /* 每分配一个块就将位图同步到硬盘 */
                block_bitmap_idx = block_lba - g_curPartion->sb->data_start_lba;
                fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
            }
            ide_write(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
        } 
    }
 
    bool first_write_block = true;  // 含有剩余空间的扇区标识
    file->fd_pos = file->fd_inode->i_size - 1;  // 置fd_pos为文件大小-1,下面在写数据时随时更新
    while (bytes_written < count) {  // 直到写完所有数据
        memset(io_buf, 0, BLOCK_SIZE);
        sec_idx = file->fd_inode->i_size / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_inode->i_size % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;

        /* 判断此次写入硬盘的数据大小 */
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;
        if (first_write_block) {
            ide_read(g_curPartion->my_disk, sec_lba, io_buf, 1);
            first_write_block = false;
        }
        memcpy(io_buf + sec_off_bytes, src, chunk_size);
        ide_write(g_curPartion->my_disk, sec_lba, io_buf, 1);

        src += chunk_size;   // 将指针推移到下个新数据
        file->fd_inode->i_size += chunk_size;  // 更新文件大小
        file->fd_pos += chunk_size;   
        bytes_written += chunk_size;
        size_left -= chunk_size;
    }
    write_inode_to_deskpart(g_curPartion, file->fd_inode, io_buf);
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_written;
}
 
 /* 从文件file中读取count个字节写入buf, 返回读出的字节数, 若到文件尾则返回-1 */
 int32_t file_read(struct file* file, void* buf, uint32_t count) {
    uint8_t* buf_dst = (uint8_t*)buf;
    uint32_t size = count, size_left = size;
 
    /* 若要读取的字节数超过了文件可读的剩余量, 就用剩余量做为待读取的字节数 */
    if ((file->fd_pos + count) > file->fd_inode->i_size)	{
        size = file->fd_inode->i_size - file->fd_pos;
        size_left = size;
        if (size == 0) {
            return -1;
        }
    }
 
    uint8_t* io_buf = sys_malloc(BLOCK_SIZE);
    if (io_buf == NULL) {
        printk("file_read: sys_malloc for io_buf failed\n");
    }
    uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);  // 用来记录文件所有的块地址
    if (all_blocks == NULL) {
        printk("file_read: sys_malloc for all_blocks failed\n");
        return -1;
    }
 
    uint32_t block_read_start_idx = file->fd_pos / BLOCK_SIZE;         // 数据所在块的起始地址
    uint32_t block_read_end_idx = (file->fd_pos + size) / BLOCK_SIZE;  // 数据所在块的终止地址
    uint32_t read_blocks = block_read_start_idx - block_read_end_idx;  // 如增量为0,表示数据在同一扇区
    ASSERT(block_read_start_idx < 139 && block_read_end_idx < 139);
    int32_t indirect_block_table;                                      // 用来获取一级间接表地址
    uint32_t block_idx;                                                // 获取待读的块地址 
 
    /* 以下开始构建all_blocks块地址数组,专门存储用到的块地址(本程序中块大小同扇区大小) */
    if (read_blocks == 0) {  // 在同一扇区内读数据,不涉及到跨扇区读取
        ASSERT(block_read_end_idx == block_read_start_idx);
    if (block_read_end_idx < 12 ) {  // 待读的数据在12个直接块之内
        block_idx = block_read_end_idx;
        all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    } else {  // 若用到了一级间接块表, 需要将表中间接块读进来
        indirect_block_table = file->fd_inode->i_sectors[12];
        ide_read(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
    }
    } else {  // 若要读多个块
        /* 第一种情况: 起始块和终止块属于直接块*/
        if (block_read_end_idx < 12) {	  // 数据结束所在的块属于直接块
            block_idx = block_read_start_idx; 
            while (block_idx <= block_read_end_idx) {
                all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx]; 
                block_idx++;
            }
       } else if (block_read_start_idx < 12 && block_read_end_idx >= 12) {
            block_idx = block_read_start_idx;
            while (block_idx < 12) {
                all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
                block_idx++;
            }
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
        } else {
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(g_curPartion->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
    }

    uint32_t sec_idx;
    uint32_t sec_lba;
    uint32_t sec_off_bytes;
    uint32_t  sec_left_bytes;
    uint32_t  chunk_size;
    uint32_t bytes_read = 0;
    while (bytes_read < size) {
        sec_idx = file->fd_pos / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_pos % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;

        memset(io_buf, 0, BLOCK_SIZE);
        ide_read(g_curPartion->my_disk, sec_lba, io_buf, 1);
        memcpy(buf_dst, io_buf + sec_off_bytes, chunk_size);

        buf_dst += chunk_size;
        file->fd_pos += chunk_size;
        bytes_read += chunk_size;
        size_left -= chunk_size;
    }
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_read;
}