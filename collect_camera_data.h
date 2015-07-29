#ifndef COLLECT_CAMERA_DATA_H
#define COLLECT_CAMERA_DATA_H
#include "trans_data.h"

struct mmap_buffer{
	void *ptr;
	int length;
};

struct addr_info{
	int length;
	int valid_count;
	void *addr;
};

struct frame_buffer{
	int r_index;
	int w_index;
	int count;
	int length;
	struct addr_info *addr_info;
};

struct frame_head{
	int frame_type;
	int frame_size;
	int timestamp;
};

struct camera_info{
	struct mmap_buffer *mmap_buf;	
	struct frame_buffer frame_buf;
	struct trans_buffer trans_buf;
};


#endif //COLLECT_CAMERA_DATA_H
