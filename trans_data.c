#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trans_data.h"

/*
struct trans_head{
	int head_size;
	int sum_size;
	int trans_number;
	int have_frame_head;
	int frame_head_offset;
};

struct trans_buffer{
	int r_pos;
	int w_pos;
	int size;
	int free;
	void *addr;
	int lock;
};
*/

int malloc_trans_buffer(struct trans_buffer *ptr)
{
	ptr->size = ((ptr->size + (1 << 12))/(1 << 12))*(1 << 12);
	ptr->free = ptr->size;
	ptr->r_pos = ptr->w_pos = 0;
	ptr->addr = (char *)malloc(ptr->size);
	printf("%d:%s ptr->size=%d\n", __LINE__, __func__, ptr->size);
	printf("%d:%s ptr->free=%d\n", __LINE__, __func__, ptr->free);
	return 0;
}

int pop_data_from_trans_buffer(void *dst, struct trans_buffer * const src, const int count)
{
	if((NULL == dst) && (NULL== src))
	{
		return -1;
	}

	//src->lock 
	if((src->size - src->free) >= count)
	{
		if((src->w_pos <= src->r_pos) && (src->size - src->r_pos) < count)
		{
			memcpy((unsigned char *)dst, (unsigned char *)src->addr + src->r_pos, src->size - src->r_pos);
			memcpy((unsigned char *)dst + (src->size - src->r_pos), (unsigned char *)src->addr, count - (src->size - src->r_pos));
		}else
		{
			memcpy((unsigned char *)dst, (unsigned char *)src->addr + src->r_pos, count);
		}
		src->r_pos += count;
		src->r_pos %= src->size;
		src->free += count;
	}
	
	//src->lock 
	//printf("%d:%s src->free=%d\n", __LINE__, __func__, src->free);
	//printf("%d:%s src->w_pos=%d\n", __LINE__, __func__, src->w_pos);
	//printf("%d:%s src->r_pos=%d\n", __LINE__, __func__, src->r_pos);
	return count;
}

