#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

//向port端口写入一个byte
static inline void outb(uint16_t port, uint8_t data)
{
	//对端口指定N表示0~255
	asm volatile ("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

//将addr起始的word_cnt个字写入port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	/*
	outsw把ds:esi处的一个字写入port
	相当于mov word dx, [ds:esi]
	*/
	asm volatile ("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port));
}

//从port读一个byte
static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm volatile ("inb %w1, %b0" : "=a"(data) : "Nd"(port));
	return data;
}

//从port读word_cnt个字到addr
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt)
{
	/*
	insw把从port读到的字写入es:edi
	相当于 mov word [es:edi], dx
	*/
	asm volatile ("cld; rep insw" : "+D"(addr), "+c"(word_cnt) : "d"(port) : "memory");
}
#endif
