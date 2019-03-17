#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

//线程中执行的函数
static void kernel_thread(thread_func* function, void* func_arg)
{
	function(func_arg);
}

//初始化线程栈
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
	//预留中断栈的空间
	pthread->self_kstack -= sizeof(struct intr_stack);
	//预留线程栈的空间
	pthread->self_kstack -= sizeof(struct thread_stack);

	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
	
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;

	kthread_stack->ebp = kthread_stack->ebx =
	kthread_stack->esi = kthread_stack->edi = 0;
}

//初始化pcb
void init_thread(struct task_struct* pthread, char* name, int prio)
{
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);
	pthread->status = TASK_RUNNING;
	pthread->priority = prio;
	//设置内核栈栈顶
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->stack_magic = 0x19870916;
}

struct task_struct* thread_start(char* name, 
								 int prio, 
								 thread_func function, 
								 void* func_arg)
{
	//get_kernel_pages()返回的是页的起始地址
	//所以现在指针指向的是pcb的最低地址
	struct task_struct* thread = get_kernel_pages(1);

	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);

	asm volatile ("movl %0, %%esp; \
				   pop %%ebp; \
				   pop %%ebx; \
				   pop %%edi; \
				   pop %%esi; \
				   ret": : "g"(thread->self_kstack) : "memory");
	return thread;
}











