#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

//用于判断用哪个 物理 内存池
enum pool_flags
{
	PF_KERNEL = 1,
	PF_USER = 2
};

/*
虚拟地址池，用于管理虚拟地址
因为同一个进程中申请内存时，是先在选择一块空闲的虚拟地址
然后在选择空闲的物理地址，最后在也表中建立映射
所以需要一个管理虚拟地址的数据结构
*/
struct virtual_addr
{
	//位图中的每一位对应一页，0表示尚未分配
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;	//虚拟地址起始地址
};

#define	 PG_P_1	  1	// 页表项或页目录项存在属性位
#define	 PG_P_0	  0	// 页表项或页目录项存在属性位
#define	 PG_RW_R  0	// R/W 属性位值, 读/执行
#define	 PG_RW_W  2	// R/W 属性位值, 读/写/执行
#define	 PG_US_S  0	// U/S 属性位值, 系统级
#define	 PG_US_U  4	// U/S 属性位值, 用户级

extern struct pool kernel_pool, user_pool;
void mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void malloc_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
#endif
