#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

//位图初始化
void bitmap_init(struct bitmap* btmp)
{
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

//判断第bit_idx(这是对整个位图的索引)为是否为1，是则返回ture否则false
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
{
	uint32_t byte_idx = bit_idx / 8;	//得到数组索引
	uint32_t bit_odd = bit_idx % 8;		//得到这个索引对应字节中的位
	return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

//在位图中申请连续cnt个位，成功则返回起始位的下标，否则返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
{
	uint32_t idx_byte = 0;	//用于记录空闲位所在的字节
	while((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len))
	{
		//1表示已被分配
		idx_byte++;
	}
	
	ASSERT(idx_byte < btmp->btmp_bytes_len);
	if(idx_byte == btmp->btmp_bytes_len)	//说明位图已经用完了
	{
		return -1;
	}

	int idx_bit = 0;	//用于记录找到的这个字节里的第一个空闲位
	while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte])
	{
		idx_bit++;
	}

	int bit_idx_start = idx_byte * 8 + idx_bit;		//空闲位在位图中的下标
	if(cnt == 1)
	{
		return bit_idx_start;
	}
	
	//记录位图中还剩多少个位可以判断 还应该再减1??
	uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start - 1);
	uint32_t next_bit = bit_idx_start + 1;
	uint32_t count = 1;		//用于记录找到的空闲位的个数 

	bit_idx_start = -1;		//若找不到就直接返回
	while(bit_left-- > 0)
	{
		if(!(bitmap_scan_test(btmp, next_bit)))
		{
			count++;
		}
		else
		{
			count = 0;	//因为是要找连续的，所以断开一个就要从头计数
		}
		if(count == cnt)
		{
			//因为中间有可能断开，所以从尾到向头算是合理的
			bit_idx_start = next_bit - cnt + 1;
			break;
		}
		next_bit++;
	}
	return bit_idx_start;
}

//将位图btmp的bit_idx位设置为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value)
{
	ASSERT((value == 0) || (value == 1));
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;

	if(value)
	{
		btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
	}
	else
	{
		btmp->bits[byte_idx] &= (~(BITMAP_MASK << bit_odd));
	}
}
