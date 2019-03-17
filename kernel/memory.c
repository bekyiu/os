#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "debug.h"
#include "string.h"

#define PG_SIZE 4096

/*
	因为0xc009f000是内核主线程栈顶，0xc009e000内核主线程的pcb
	一个页框大小的位图可以表示128MB内存，位图放在0xc009a000
	这样本系统支持最4个页框的位图 512MB
*/
#define MEM_BITMAP_BASE 0xc009a000

//用来表示内核使用的堆的虚拟地址
//因为低端1mb基本上占满了 所以跨过
#define K_HEAP_START 0xc0100000

//用于返回虚拟地址高10位
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
//中间10位
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

//内存池 用来管理物理内存
struct pool
{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;		//物理内存是有大小的 单位byte

};

struct pool kernel_pool, user_pool;		//内核池，用户池
struct virtual_addr kernel_vaddr;		//给内核分配虚拟地址

//在pf表示的虚拟内存池中申请pg_cnt个虚拟页, 成功返回虚拟页的起始地址
//否则返回NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if(pf == PF_KERNEL)
	{
		//在管理虚拟地址的位图中连续申请pg_cnt个单位
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
		{
			return NULL;
		}
		while(cnt < pg_cnt)
		{
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}
	else
	{
		//todo 用户内存池
	}
	return (void*)vaddr_start;
}

//返回一个可以访问vaddr所在pte的虚拟地址
uint32_t* pte_ptr(uint32_t vaddr)
{
	uint32_t* pte = (uint32_t*)
	(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}

//返回一个可以访问vaddr所在pde的虚拟地址
uint32_t* pde_ptr(uint32_t vaddr)
{
	uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
	return pde;
}
/*
	注意：以上两个函数返回的都是虚拟地址，但是这个虚拟地址进过分页地址转换之后所
	指向的是vaddr所在的pte或pde的物理地址
*/


/*
	在m_pool指向的物理内存池中分配一个物理页
	成功返回页框的物理地址，失败则返回NULL
*/
static void* palloc(struct pool* m_pool)
{
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(bit_idx == -1)
	{
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
	return (void*)page_phyaddr;
}

//页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr)
{
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);
	
	//p位为1,说明该页表存在
	if(*pde & 0x00000001)
	{
		ASSERT(!(*pte & 0x00000001));
		if(!(*pte & 0x00000001))//如果页表项不存在
		{
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}
		else
		{
			PANIC("pte repeat");
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}
	}
	else//先创建页表,再往pte中写物理地址
	{
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		/*
		分配到的页表应该清0，要想初始化，必须的有页表的虚拟地址
		但pde_phyaddr是个物理地址，所以不能用它。
		此页表的虚拟地址应该是 pte的后12为置0即可
		*/
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		ASSERT(!(*pte & 0x00000001));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}

//申请pg_cnt个虚拟页，并为他分配物理页，在页表中建立好映射
//成功返回起始虚拟地址，否则返回null
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);
	void* vaddr_start = vaddr_get(pf, pg_cnt);
	if(vaddr_start == NULL)
	{
		return NULL;
	}

	uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
	struct pool* mem_pool = 
	((pf - PF_KERNEL) == 0) ?  &kernel_pool : &user_pool;

	while(cnt-- > 0)
	{
		void* page_phyaddr = palloc(mem_pool);
		if(page_phyaddr == NULL)
		{	
			//失败的时候还应将已经申请的虚拟地址和物理地址回滚
			//以后在说
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

//从物理内存中申请pg_cnt页内存，返回虚拟地址
void* get_kernel_pages(uint32_t pg_cnt)
{
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL)
	{
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	return vaddr;
}

//初始化内存池
static void mem_pool_init(uint32_t all_mem)
{
	put_str("	mem_pool_init start\n");
	//页表大小 = 1页页目录表 + 第0和768页目录项指向的同一个页表 + 
	//第769~1022个页目录项指向的254个页表 
	uint32_t page_table_size = PG_SIZE * 256;	//共1M
	uint32_t used_mem = page_table_size + 0x100000;//低端1mb
	uint32_t free_mem = all_mem - used_mem;

	//还有多少页可用, 粗粒度 不足一页的内存不考虑了
	uint16_t all_free_pages = free_mem / PG_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;

	//kernel bitmap长度 以字节为单位 一位表示一页
	uint32_t kbm_length = kernel_free_pages / 8;
	//user bitmap 长度
	uint32_t ubm_length = user_free_pages / 8;
	//内核内存池的起始地址
	uint32_t kp_start = used_mem;
	//用户内存池的起始地址
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

	//初始化开始
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

	//输出内存池信息
	put_str("		kernel_pool_bitmap_start: ");
	put_int((int)kernel_pool.pool_bitmap.bits);
	put_str(" kernel_pool_phy_addr_start: ");
	put_int(kernel_pool.phy_addr_start);
	put_str("\n");

	put_str("		user_pool_bitmap_start:");
	put_int((int)user_pool.pool_bitmap.bits);
	put_str(" user_pool_phy_addr_start:");
	put_int(user_pool.phy_addr_start);
	put_str("\n");

	//将位图置0
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);
	
	//初始化内核虚拟地址的位图
	//用于维护内核堆的虚拟地址，所以要和内核内存池大小一致
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

	kernel_vaddr.vaddr_bitmap.bits = 
	(void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("	mem_pool_init done\n");
}

//内存管理部分初始化入口
void mem_init()
{
	put_str("mem_init_start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done \n");
}












