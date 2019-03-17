#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
typedef void thread_func(void*);

enum task_status
{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

//中断栈 用于中断发生时保护进程或线程的上下文 在页的最顶端
struct intr_stack
{
	uint32_t vec_no;	 //kernel.S 宏VECTOR中push %1压入的中断号
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;	 //虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	/* 以下由cpu从低特权级进入高特权级时压入 */
	uint32_t err_code;		 // err_code会被压入在eip之后
	void (*eip) (void);
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss;
};


//线程栈 用于存储线程中待执行的函数
struct thread_stack
{
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;

	//线程第一次执行时，eip指向待调用的函数kernel_thread
	//其他时候eip指向switch_to的返回地址
	void (*eip) (thread_func* func, void* func_arg);

	//以下仅供第一次上cpu的时候使用
	void (*unused_retaddr);
	thread_func* function;
	void* func_arg;
};

//pcb
struct task_struct
{
	uint32_t* self_kstack;	//各内核线程自己的内核栈
	enum task_status status;
	uint8_t priority;
	char name[16];
	uint32_t stack_magic;	//为了不让栈疯狂向下涨的一个界限

};


void thread_create(struct task_struct* pthread, 
				   thread_func function, 
				   void* func_arg);

void init_thread(struct task_struct* pthread, char* name, int prio);

struct task_struct* thread_start(char* name, 
								 int prio, thread_func function, 
								 void* func_arg);

#endif


