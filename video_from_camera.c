
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#define PATH	"/dev/video0"
//#define PATH	"/dev/usb/hiddev0"
#define PIX_WIDTH	640
#define PIX_HEIGHT	480

struct mmap_buffer{
	void *ptr;
	int length;
};
struct buffer{
	int length;
	void *addr;		
};
struct fifo_buffer{
	int r_index;
	int w_index;
	int size;
	int free;
	void *addr;
	int lock;
};
struct camera_info{
	struct mmap_buffer *mmap_buf;	
	struct fifo_buffer fifo_buf;

};

struct camera_info camera;

void get_v4l2_capability(int fd)
{
	int ret = -1;
	struct v4l2_capability cap;
	ret = ioctl(fd,	VIDIOC_QUERYCAP, &cap);
	if(ret < 0)
	{
		printf("%d: %s failed\n", __LINE__, __func__);	

	}else
	{		
		printf("driver=%s, card=%s bus_info=%s, version=%d, capabilities=%d, device_caps=%d, reserver=%s\n", cap.driver, cap.card, cap.bus_info, cap.version, cap.capabilities, cap.device_caps, "cap.reserver");
	}

	if(!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE))
	{
		printf("%d: %s The Current device is not a video capture device\n", __LINE__, __func__);	
	}
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("%d: %s The Current device does not support streaming i/o\n", __LINE__, __func__);	
	}
}

void get_v4l2_fmtdesc(int fd)
{
	int ret = -1;
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0)
	{
		fmtdesc.index ++;	
	}
	printf("flags=%d, description=%s, pixelformat=%d\n", fmtdesc.flags, fmtdesc.description, fmtdesc.pixelformat);
}

void get_v4l2_format(int fd)
{
	int ret = -1;
	struct v4l2_format format;

	ret = ioctl(fd, VIDIOC_G_FMT, &format);
	if(ret < 0)
			printf("%d:%s fail\n", __LINE__, __func__);

	switch(format.fmt.pix.pixelformat)
	{
		case V4L2_PIX_FMT_YVU410:
			printf("%d:%s \n", __LINE__, __func__);
				break;

		case V4L2_PIX_FMT_YVU420:
			printf("%d:%s V4L2_PIX_FMT_YVU420\n", __LINE__, __func__);
				break;

		case V4L2_PIX_FMT_YUYV:
			printf("%d:%s V4L2_PIX_FMT_YUYV\n", __LINE__, __func__);
				break;

		case V4L2_PIX_FMT_YUV422P:
			printf("%d:%s V4L2_PIX_FMT_YUV422P\n", __LINE__, __func__);
				break;

		case V4L2_PIX_FMT_YUV420:
			printf("%d:%s V4L2_PIX_FMT_YUV420\n", __LINE__, __func__);
				break;

		default:
				break;
	}
	
	printf("%d:%s format.fmt.pix.width=%d\n", __LINE__, __func__, format.fmt.pix.width);
	printf("%d:%s format.fmt.pix.height=%d\n", __LINE__, __func__, format.fmt.pix.height);
}

void set_v4l2_format(int fd)
{
	int ret = -1;
	struct v4l2_format format;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = PIX_WIDTH;
	format.fmt.pix.height = PIX_HEIGHT;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_YUV420 V4L2_PIX_FMT_YUV422P
	//format.fmt.pix.priv = V4L2_FMT_OUT;
	format.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if(ret < 0)
			printf("%d:%s fail\n", __LINE__, __func__);
}

int set_frame_fp(int fd, int fps)
{
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
	parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = fps;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		printf("%d:%s fail\n", __LINE__, __func__);
	return -1;
	}

	return 1;
}

int get_frame_fp(int fd)
{
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (ioctl(fd, VIDIOC_G_PARM, &parm) < 0) {
		printf("%d:%s fail\n", __LINE__, __func__);
	return -1;
	}

	printf("%d:%s parm.parm.capture.timeperframe.numerator=%d\n", __LINE__, __func__, parm.parm.capture.timeperframe.numerator);
	printf("%d:%s parm.parm.capture.timeperframe.denominator=%d\n", __LINE__, __func__, parm.parm.capture.timeperframe.denominator);
	return 0;
}

int get_v4l2_requestbuffers(int fd, int buf_count)
{
	int ret = -1;
	struct v4l2_requestbuffers reqbuf;

	bzero(&reqbuf, sizeof(struct v4l2_requestbuffers));
	reqbuf.count = buf_count;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0)
	{
		printf("%d:%s fail\n", __LINE__, __func__);
		return -1;
	}else
	{
		printf("%d:%s count=%d\n", __LINE__, __func__, reqbuf.count);
		return reqbuf.count;
	}
}

int mmap_v4l2_buffer(const int fd, int count, struct camera_info *ca_info)
{
	int ret = -1, i = 0, buf_size = 0;
	void *ptr = NULL;

	if(count < 1)
			return -1;
	ca_info->mmap_buf = (struct mmap_buffer *)malloc(count*sizeof(struct mmap_buffer));
	
	for(i = 0; i < count; i++)
	{
		struct v4l2_buffer buf;
		
		bzero(&buf, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		printf("%d:%s i=%d buf.length=%d\n", __LINE__, __func__, i, buf.length);
		if(ret < 0)
		{
			printf("%d:%s i=%d ioctl fail\n", __LINE__, __func__, i);
			return -2;
		}
		ptr = mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if(MAP_FAILED == (void *)ptr)
		{
			printf("%d:%s i=%d mmap fail\n", __LINE__, __func__, i);
			return -2;
		}
		(ca_info->mmap_buf + i)->ptr = ptr;
		(ca_info->mmap_buf + i)->length = buf.length;
		buf_size += buf.length;
		printf("%d:%s ptr=%p\n", __LINE__, __func__, ptr);
		printf("%d:%s buf_size=%d\n", __LINE__, __func__, buf_size);
		ptr = NULL;
	}
	printf("%d:%s buf_size=%d\n", __LINE__, __func__, buf_size);

	ca_info->fifo_buf.size = ((buf_size + (1 << 12))/(1 << 12))*(1 << 12);
	ca_info->fifo_buf.free = ca_info->fifo_buf.size;
	ca_info->fifo_buf.r_index = ca_info->fifo_buf.w_index = 0;
	ca_info->fifo_buf.addr = (char *)malloc(ca_info->fifo_buf.size);
	printf("%d:%s ca_info->fifo_buf.size=%d\n", __LINE__, __func__, ca_info->fifo_buf.size);
	printf("%d:%s ca_info->fifo_buf.free=%d\n", __LINE__, __func__, ca_info->fifo_buf.free);
	
	if(ret < 0)
	{
		printf("%d:%s fail\n", __LINE__, __func__);
		return -2;
	}else
	{
		return 0;	
	}
}

int v4l2_buffer_VIDIOC_QBUF(int fd, int index)
{
	struct v4l2_buffer buf;
	int ret = -1;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;
	
	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if(ret < 0)
	{
		printf("%d:%s ioctl fail\n", __LINE__, __func__);
	}else if(0 == ret)
	{
		//printf("%d:%s ioctl success\n", __LINE__, __func__);
		printf("%d:%s ioctl success index=%d\n", __LINE__, __func__, buf.index);
	}

	return ret;
}

int push_data_to_fifo_buffer(struct fifo_buffer *dst, const struct mmap_buffer *src, const index)
{
	if((NULL == dst) && (NULL== src))
	{
		return -1;
	}

	//src->lock 
	if(dst->free >= (src + index)->length)
	{
		if((dst->r_index <= dst->w_index) && ((dst->size - dst->w_index) < (src + index)->length))
		{
			printf("%d:%s\n", __LINE__, __func__);
			unsigned char *temp = (unsigned char *)malloc((src + index)->length);
			if(temp == NULL)
			{
				printf("%d:%s malloc temp failed\n", __LINE__, __func__);
			}
			memcpy(temp, (unsigned char *)((src + index)->ptr), (src + index)->length);
			memcpy((unsigned char *)dst->addr + dst->w_index, temp, dst->size - dst->w_index);
			memcpy((unsigned char *)dst->addr, temp + (dst->size - dst->w_index), (src + index)->length - (dst->size - dst->w_index));
			free(temp);
		}else
		{
			memcpy((unsigned char *)dst->addr + dst->w_index, (unsigned char *)((src + index)->ptr), (src + index)->length);
		}
		dst->w_index += (src + index)->length;
		dst->w_index %= dst->size;
		dst->free -= (src + index)->length;
	
		//printf("%d:%s dst->free=%d\n", __LINE__, __func__, dst->free);
		//printf("%d:%s (src + index)->length=%d\n", __LINE__, __func__, (src + index)->length);
		//printf("%d:%s dst->w_index=%d\n", __LINE__, __func__, dst->w_index);
		//printf("%d:%s dst->r_index=%d\n", __LINE__, __func__, dst->r_index);		
	}

	
	//src->lock 
	return (src + index)->length;
}

int pop_data_from_fifo_buffer(void *dst, struct fifo_buffer * const src, const int count)
{
	if((NULL == dst) && (NULL== src))
	{
		return -1;
	}

	//src->lock 
	if((src->size - src->free) >= count)
	{
		if((src->w_index <= src->r_index) && (src->size - src->r_index) < count)
		{
			memcpy((unsigned char *)dst, (unsigned char *)src->addr + src->r_index, src->size - src->r_index);
			memcpy((unsigned char *)dst + (src->size - src->r_index), (unsigned char *)src->addr, count - (src->size - src->r_index));
		}else
		{
			memcpy((unsigned char *)dst, (unsigned char *)src->addr + src->r_index, count);
		}
		src->r_index += count;
		src->r_index %= src->size;
		src->free += count;
	}
	
	//src->lock 
	//printf("%d:%s src->free=%d\n", __LINE__, __func__, src->free);
	//printf("%d:%s src->w_index=%d\n", __LINE__, __func__, src->w_index);
	//printf("%d:%s src->r_index=%d\n", __LINE__, __func__, src->r_index);
	return count;
}

int v4l2_buffer_VIDIOC_DQBUF_QBUF(int fd, struct camera_info *ca_info)
{
	struct v4l2_buffer buf;
	int ret = -1; 
	unsigned char i;
	//lock
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	
	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	if(ret < 0)
	{
		printf("%d:%s ioctl fail\n", __LINE__, __func__);
	}else if(0 == ret)
	{
		printf("%d:%s ioctl success Dindex=%d\n", __LINE__, __func__, buf.index);
	}

//	printf("%d:%s ca_info->fifo_buf.w_index=%d\n", __LINE__, __func__, ca_info->fifo_buf.w_index);
//	printf("%d:%s ca_info->mmap_buf->length=%d\n", __LINE__, __func__, ca_info->mmap_buf->length);

	push_data_to_fifo_buffer(&ca_info->fifo_buf, ca_info->mmap_buf, buf.index);

	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if(ret < 0)
	{
		printf("%d:%s ioctl fail\n", __LINE__, __func__);
	}else if(0 == ret)
	{
		printf("%d:%s ioctl success Qindex=%d\n", __LINE__, __func__, buf.index);
	}
	//unlock
	return ret;
}

int v4l2_buffer_VIDIOC_STREAMON(int fd)
{
	int ret = -1;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if(ret < 0)
	{
		printf("%d:%s ioctl fail\n", __LINE__, __func__);
	}else if(0 == ret)
	{
		printf("%d:%s ioctl success\n", __LINE__, __func__);
	}

}

void get_v4l2_framebuffer(int fd)
{
	int ret = -1;
	struct v4l2_framebuffer framebuf;
	ret = ioctl(fd, VIDIOC_G_FBUF, &framebuf);
	if(ret < 0)
			printf("%d:%s fail\n", __LINE__, __func__);

}

void set_v4l2_framebuffer(int fd)
{
	int ret = -1;
	struct v4l2_framebuffer framebuf;
	ret = ioctl(fd, VIDIOC_S_FBUF, &framebuf);
	if(ret < 0)
			printf("%d:%s fail\n", __LINE__, __func__);

}

int open_camera(void)
{
	int fd = -1, ret = -1, index;
	fd = open(PATH, O_RDWR | O_NONBLOCK);

	printf("fd = %d\n", fd);
	if(fd > 0)
	{
		printf("open successful\n");
	}else
	{
		fd = open(PATH, O_RDONLY);
		if(fd > 0)
		{
			printf("open successful\n");
		}else
		{
			printf("open fail\n");
			return -1;
		}
		
	}

	return fd;
}

void init_param(int fd)
{
	int ret = -1, index;
	memset(&camera, 0, sizeof(camera));
	
	get_v4l2_capability(fd);
	set_v4l2_format(fd);
	set_frame_fp(fd, 30);
	get_v4l2_format(fd);
	get_frame_fp(fd);
	ret = get_v4l2_requestbuffers(fd, 4);
	mmap_v4l2_buffer(fd, ret, &camera);
	for(index = 0; index < ret; index++)
	{
		v4l2_buffer_VIDIOC_QBUF(fd, index);
	}
	v4l2_buffer_VIDIOC_STREAMON(fd);
}


void start_capture_data(int fd)
{
	int epollfd, nfds;
	struct epoll_event ev, events[10];
	epollfd = epoll_create(4);
	if(-1 == epollfd)
	{
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}
	ev.events = EPOLLIN;
	ev.data.fd = fd; 
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		perror("epoll_ctl: fd");
		exit(EXIT_FAILURE);
	}

	unsigned char temp_buf[50000], i;
	for(;;)
	{
		nfds = epoll_wait(epollfd, events, 10, 1000);
		if(nfds == -1)
		{
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		printf("nfds = %d\n", nfds);
		v4l2_buffer_VIDIOC_DQBUF_QBUF(fd, &camera);
		pop_data_from_fifo_buffer(temp_buf, &camera.fifo_buf, 50000);

		write_file("chenwenmin.yuv", temp_buf, 50000);
		#if 0
		for(i = 0; i < 256; i++)
		{
			printf("temp_buf[%d] = %d\n", i, temp_buf[i]);
		}
		#endif
		
		//v4l2_buffer_VIDIOC_QBUF(fd, index);
	}
}

int close_camera(int fd)
{
	int ret = -1;
	ret = close(fd);
	printf("ret = %d\n", ret);
	if(ret < 0)
			printf("close camera failed\n");

	return ret;
}

