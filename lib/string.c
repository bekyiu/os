#include "string.h"
#include "global.h"
#include "debug.h"

//将dst_起始的size个字节置为value
void memset(void* dst_, uint8_t value, uint32_t size)
{
	ASSERT(dst_ != NULL);
	uint8_t* dst = dst_;
	while(size-- > 0)
	{
		*dst++ = value;
	}
}

//将src_起始的size个字节复制到dst_
void memcpy(void* dst_, const void* src_, uint32_t size)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t* dst = dst_;
	const uint8_t* src = src_;
	while(size-- > 0)
	{
		*dst++ = *src++;
	}
}

//连续比较以地址a_和b_开头的size个字节, 相等返回0, *a_ > *b返回1, 小于返回-1
int memcmp(const void* a_, const void* b_, uint32_t size)
{
	const char* a = a_;
	const char* b = b_;
	ASSERT(a != NULL || b != NULL);
	while(size-- >0)
	{
		if(*a != *b)
		{
			return *a > *b ? 1 : -1;
		}
		a++;
		b++;
	}
	return 0;
}

//将字符串从src_复制到dst_
char* strcpy(char* dst_, const char* src_)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* r = dst_;		// to return
	while((*dst_++ = *src_++));
	return r;
}

//返回字符串长度
uint32_t strlen(const char* str)
{
	ASSERT(str != NULL);
	int len = 0;
	while(*str++)
	{
		len++;
	}
	return len;
}

//比较两个字符串 若a_中的字符大于b_中的字符返回1 相等返回0 否则返回-1
int8_t strcmp(const char* a, const char* b)
{
	ASSERT(a != NULL && b != NULL);
	while(*a != 0 && *a == *b)
	{
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}

//从左到右查找字符串str中首次出现字符ch的地址
char* strchr(const char* str, const uint8_t ch)
{
	ASSERT(str != NULL);
	while(*str != 0)
	{
		if(*str == ch)
		{
			return (char*)str;
		}
		str++;
	}
	return NULL;
}

//从后往前查找str中首次出现ch的地址
char* strrchr(const char* str, const uint8_t ch)
{
	ASSERT(str != NULL);
	const char* last_char = NULL;
	while(*str != 0)
	{
		if(*str == ch)
		{
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

//将src_拼接到dst_后，返回拼接的串地址
char* strcat(char* dst_, const char* src_)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* str = dst_;
	while(*str++);
	str--;
	while((*str++ = *src_++));
	return dst_;
}

//在str中查找ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch)
{
	ASSERT(str != NULL);
	uint32_t count = 0;
	while(*str)
	{
		if(*str == ch)
		{
			count++;
		}
		str++;
	}
	return count;
}
