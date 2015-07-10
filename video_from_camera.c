
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


#define PATH	"/dev/video0"
//#define PATH	"/dev/usb/hiddev0"
#define FIFO_COUNT	8
#define FIFO_LENGTH	15000
struct mmap_buffer{
	void *ptr;
	int length;
};
struct buffer{
	int length;
	void *addr;		
};
struct fifo_buffer{
	char r_index;
	char w_index;
	char count;
	struct buffer *buf;
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
}

void set_v4l2_format(int fd)
{
	int ret = -1;
	struct v4l2_format format;
	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = 640;
	format.fmt.pix.height = 480;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	format.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if(ret < 0)
			printf("%d:%s fail\n", __LINE__, __func__);
}

set_frame_fp(int fd, int fps)
{
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = fps;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		printf("%d:%s fail\n", __LINE__, __func__);
	return 0;
	}

	return 1;
}

int get_v4l2_requestbuffers(int fd, int buf_count)
{
	int ret = -1;
	struct v4l2_requestbuffers reqbuf;
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

int get_v4l2_buffer(const int fd, int count, struct camera_info *ca_info)
{
	int ret = -1, i = 0;
	struct v4l2_buffer buf;
	void *ptr = NULL;

	if(count < 1)
			return -1;
	ca_info->mmap_buf = (struct mmap_buffer *)malloc(count*sizeof(struct mmap_buffer));
	ca_info->fifo_buf.count = count;
	ca_info->fifo_buf.buf = (struct buffer *)malloc(ca_info->fifo_buf.count*sizeof(struct buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	for(i = 0; i < count; i++)
	{
		buf.index = i;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		printf("%d:%s i=%d buf.length=%d\n", __LINE__, __func__, i, buf.length);
	//	printf("%d:%s i=%d buf.m.offset=%d\n", __LINE__, __func__, i, buf.m.offset);
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
		(ca_info->fifo_buf.buf + i)->length = buf.length;
		(ca_info->fifo_buf.buf + i)->addr = (void *)malloc((ca_info->fifo_buf.buf + i)->length);
		printf("%d:%s ptr=%p\n", __LINE__, __func__, ptr);
		ptr = NULL;
	}
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

int v4l2_buffer_VIDIOC_DQBUF_QBUF(int fd, struct camera_info *ca_info)
{
	struct v4l2_buffer buf;
	int ret = -1;
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
	
	if(((ca_info->fifo_buf.w_index+1)%ca_info->fifo_buf.count) != ca_info->fifo_buf.r_index)
	{
		memcpy((char *)((ca_info->fifo_buf.buf + ca_info->fifo_buf.w_index)->addr), (char *)((ca_info->mmap_buf+buf.index)->ptr), (ca_info->fifo_buf.buf + ca_info->fifo_buf.w_index)->length);
		ca_info->fifo_buf.w_index++;
		ca_info->fifo_buf.w_index %= ca_info->fifo_buf.count;
		printf("%d:%s memcpy\n", __LINE__, __func__);
	}
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
	fd = open(PATH, O_RDWR);

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
	camera.mmap_buf = NULL;
	camera.fifo_buf.r_index = 0;
	camera.fifo_buf.w_index = 0;
	
	set_v4l2_format(fd);
	set_frame_fp(fd, 30);
	ret = get_v4l2_requestbuffers(fd, 4);
	get_v4l2_buffer(fd, ret, &camera);
	get_v4l2_capability(fd);
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

