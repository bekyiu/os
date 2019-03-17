#include "print.h"
#include "init.h"
#include "thread.h"

void k_thread_a(void*);

int main(void)
{
	put_str("i am the kernel, fuck you\n");
	init_all();
	
	thread_start("k_thread_a", 31, k_thread_a, "Louise");
	
	while(1);
	return 0;
}

void k_thread_a(void* arg)
{
	char* str = arg;
	while(1)
	{
		put_str(str);
	}

}
