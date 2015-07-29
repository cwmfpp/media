#ifndef TRANS_DATA_H
#define TRANS_DATA_H

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


int malloc_trans_buffer(struct trans_buffer *ptr);

#endif //TRANS_DATA_H
