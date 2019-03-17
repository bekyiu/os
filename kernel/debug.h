#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
void panic_spin(char* filename, int line, const char* func, const char* condition);
/*
	... 表示可变参数
	后面函数的参数都是编译器的内置宏
*/
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__);

#ifdef NDEBUG
	#define ASSERT(CONDITION) ((void)0)
#else
	#define ASSERT(CONDITION)	\
	if(CONDITION){}				\
	else						\
	{							\
		PANIC(#CONDITION);		\
	}							\

#endif	//ndebug end

#endif	//kernel debug end
