#include "exec.h"
#include "stdint.h"
#include "string.h"
#include "fs.h"
#include "memory.h"
#include "thread.h"

typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Half;

extern void interrupt_exit(void);

/* 32位 ELF 头 */
struct Elf32_Ehdr {
    unsigned char e_ident[16];     // 魔数和标识信息
    Elf32_Half e_type;             // 文件类型
    Elf32_Half e_machine;          // 目标架构
    Elf32_Word e_version;          // ELF 版本
    Elf32_Addr e_entry;            // 入口点地址
    Elf32_Off  e_phoff;            // 程序头表偏移量
    Elf32_Off  e_shoff;            // 节区表偏移量
    Elf32_Word e_flags;            // 特殊标志
    Elf32_Half e_ehsize;           // ELF 头大小
    Elf32_Half e_phentsize;        // 每个程序头条目的大小
    Elf32_Half e_phnum;            // 程序头条目数量
    Elf32_Half e_shentsize;        // 每个节区头条目的大小
    Elf32_Half e_shnum;            // 节区头目数量
    Elf32_Half e_shstrndx;         // 节区名称字符串表索引
};

/* 32位程序头 */ 
struct Elf32_Phdr {
    Elf32_Word p_type;   // 段类型
    Elf32_Off  p_offset; // 文件中的偏移量
    Elf32_Addr p_vaddr;  // 内存中的虚拟地址
    Elf32_Addr p_paddr;  // 物理地址（一般不用）
    Elf32_Word p_filesz; // 段在文件中的大小
    Elf32_Word p_memsz;  // 段在内存中的大小
    Elf32_Word p_flags;  // 段的标志位 (读写执行)
    Elf32_Word p_align;  // 段的对齐方式
} Elf32_Phdr;

/* 段类型 */
enum segment_type {
    PT_NULL,     // 忽略
    PT_LOAD,     // 可加载程序段
    PT_DYNAMIC,  // 动态加载信息 
    PT_INTERP,   // 动态加载器名称
    PT_NOTE,     // 一些辅助信息
    PT_SHLIB,    // 保留
    PT_PHDR      // 程序头表
};

/* 加载段：在fd指向的文件中读取偏移为 offset，大小为 filesz 的段，将其加载到虚拟地址 vaddr 指向的内存 */
static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
    /* 首个页框地址 */
    uint32_t page_vaddr = vaddr & 0xfffff000;
    /* 首个页框剩余空闲字节数 */
    uint32_t left_size_in_page = PAGE_SIZE - (vaddr & 0x00000fff);
    /* 所需页框数 */
    uint32_t page_cnt = 0;

    /* 计算需要的页数 */
    if (filesz > left_size_in_page) {
        uint32_t left_size = filesz - left_size_in_page;
        page_cnt = (left_size + PAGE_SIZE - 1) / PAGE_SIZE + 1;
    } else {
        page_cnt = 1;
    }

    /* 为进程分配内存 */
    uint32_t index = 0;
    uint32_t vaddr_page = page_vaddr;
    while (index < page_cnt) {
        uint32_t* pde = get_pde_ptr(vaddr_page);
        uint32_t* pte = get_pte_ptr(vaddr_page);
        /* 若页表存在，则直接覆盖；不存在则获取一页 */
        if (!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
            if (bind_vaddr_with_mempool(POOL_FLAG_USER, vaddr_page) == NULL) {
                return false;
            }
        }
        vaddr_page += PAGE_SIZE;
        index++;
    }
    sys_lseek(fd, offset, SEEK_SET);
    sys_read(fd, (void*)vaddr, filesz);
    return true;
}

/* 加载用户程序: 成功则返回程序的起始地址, 失败返回-1 */
static int32_t load(const char* pathname) {
    int32_t ret = -1;
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));

    int32_t fd = sys_open(pathname, O_RDONLY);
    if (fd == -1) {
       return -1;
    }

    if (sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr)) {
        ret = -1;
        goto done;
    }

    /* 校验 ELF 头 */
    if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) \
        || (elf_header.e_type != 2) \
        || (elf_header.e_machine != 3) \
        || (elf_header.e_version != 1) \
        || (elf_header.e_phnum > 1024) \
        || (elf_header.e_phentsize != sizeof(struct Elf32_Phdr))) {
        ret = -1;
        goto done;
    }

    Elf32_Off ph_offset = elf_header.e_phoff; 
    Elf32_Half ph_size = elf_header.e_phentsize;

    /* 遍历所有程序头 */
    uint32_t index = 0;
    while (index < elf_header.e_phnum) {
        memset(&prog_header, 0, ph_size);
        /* 将文件的指针定位到程序头 */
        sys_lseek(fd, ph_offset, SEEK_SET);
        /* 获取程序头 */
        if (sys_read(fd, &prog_header, ph_size) != ph_size) {
            ret = -1;
            goto done;
        }
 
        /* 将可加载段加载到内存 */
        if (PT_LOAD == prog_header.p_type) {
            if (!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
                ret = -1;
                goto done;
            }
        }

        /* 更新下一个程序头的偏移 */
        ph_offset += elf_header.e_phentsize;
        index++;
    }
    ret = elf_header.e_entry;
done:
    sys_close(fd);
    return ret;
}

int32_t sys_execv(const char* path, const char* argv[]) {
    uint32_t argc = 0;
    while (argv[argc]) {
       argc++;
    }
    int32_t entry_point = load(path);     
    if (entry_point == -1) {	 // 若加载失败则返回-1
       return -1;
    }

    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
    /* 修改进程名 */
    memcpy(cur_thread_pcb->name, path, PROCESS_NAME_MAX_LEN);

    /* 修改栈中参数 */
    struct INTR_STACK* intr_0_stack = (struct INTR_STACK*)((uint32_t)cur_thread_pcb + PAGE_SIZE - sizeof(struct INTR_STACK));
    /* 参数传递给用户进程 */
    intr_0_stack->ebx = (int32_t)argv;
    intr_0_stack->ecx = argc;
    intr_0_stack->eip = (void*)entry_point;
    /* 使新用户进程的栈地址为最高用户空间地址 */
    intr_0_stack->esp = (void*)0xc0000000;

    /* exec不同于fork,为使新进程更快被执行, 直接从中断返回 */
    asm volatile ("movl %0, %%esp; jmp interrupt_exit" : : "g" (intr_0_stack) : "memory");
    return 0;
}