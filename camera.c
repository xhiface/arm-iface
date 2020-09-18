/*
 *1：以V4L2_PIX_FMT_MJPEG方式采集一张mjpeg格式的图片，保存在本地
 *2：代码仅仅测试
 */

#include <stdio.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>  
#include <string.h>   
#include <assert.h>  
#include <getopt.h>             
#include <fcntl.h>              
#include <unistd.h>  
#include <errno.h>  
#include <malloc.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
#include <linux/fb.h>
#include <asm/types.h>          
#include <linux/videodev2.h>  
#include <signal.h>
#include <setjmp.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define CAMERA_DEVICE 	"/dev/video0" 
#define CLEAR(x) memset (&(x), 0, sizeof (x))  
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480  
#define BUFFER_COUNT 2
#define XS 95

typedef struct VideoBuffer { 
	 void *start;    //视频缓冲区的起始地址
	 size_t length;  
} VideoBuffer;

struct v4l2_capability cap;
VideoBuffer framebuf[BUFFER_COUNT];
struct v4l2_format fmt;
struct v4l2_fmtdesc fmtdesc;
struct v4l2_requestbuffers reqbuf;
struct v4l2_buffer buf;
int fd;
/*unsigned char *starter;

typedef unsigned char byte;

	byte *intToByte(int val) {
    byte buf[4];
    buf[0] = val >> 24;
    buf[1] = val >> 16;
    buf[2] = val >> 8;
    buf[3] = val;
    return buf;
	}*/

int camera_init()
{
    int ret;
	fd = open(CAMERA_DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Open %s failed\n", CAMERA_DEVICE);
		exit(-1);
	}
	printf("open device ok\n");

    //get
	fmtdesc.index=0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
	{
		fmtdesc.index++;
		ret=ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
	}
	printf("--------VIDIOC_ENUM_FMT---------\n");
	printf("get the format what the device support\n{ pixelformat = ''%c%c%c%c'', description = ''%s'' }\n",fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF,(fmtdesc.pixelformat >> 24) & 0xFF, fmtdesc.description);

    //set 
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = VIDEO_WIDTH;
	fmt.fmt.pix.height      = VIDEO_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//here is key code 1 ：chose from （V4L2_PIX_FMT_YUYV;V4L2_PIX_FMT_MJPEG）
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		printf("VIDIOC_S_FMT failed (%d)\n", ret);
		exit(-1);
	}
    printf("------------VIDIOC_S_FMT---------------\n");
	printf("Stream Format Informations:\n");
	printf(" type: %d\n", fmt.type);
	printf(" width: %d\n", fmt.fmt.pix.width);
	printf(" height: %d\n", fmt.fmt.pix.height);

	reqbuf.count = BUFFER_COUNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0) {
		printf("VIDIOC_REQBUFS failed (%d)\n", ret);
		exit(-1);
	}
    
    //map
	int i;
	for (i = 0; i < reqbuf.count; i++)
	{
		CLEAR (buf);
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd , VIDIOC_QUERYBUF, &buf);//buf取得内存缓冲区的信息
		if(ret < 0) {
			printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
		    exit(-1);
		}
		framebuf[i].length = buf.length;
		framebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (framebuf[i].start == MAP_FAILED) {
			printf("mmap (%d) failed: %s\n", i, strerror(errno));
		    exit(-1);
		}
		ret = ioctl(fd , VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
		    exit(-1);
		}
	}
}

void camera_start()
{  
    int ret;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//设置采集方式
	ret = ioctl(fd, VIDIOC_STREAMON, &type);//帧缓存入队
	if (ret < 0) {
		printf("VIDIOC_STREAMON failed (%d)\n", ret);
        exit(-1);
	}
	printf("---camera start--\n");

}

int store_pic(int connfd)
{
	char filesize[10];
	int ret;
	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	if (ret < 0) {
		printf("VIDIOC_DQBUF failed (%d)\n", ret);
		exit(-1);
	}

	//1.send filesize
	
	snprintf(filesize,10,"%d",buf.bytesused);
	write(connfd,filesize,10);
	printf("send %d\n",buf.bytesused);

	//2.send filedata
	write(connfd,framebuf[buf.index].start,buf.bytesused);

    
	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if (ret < 0) {
		printf("VIDIOC_QBUF failed (%d)\n", ret);
		exit(-1);
	}
}

int camera_stop()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//设置采集方式
	//停止采集
	if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)){
		printf("VIDIOC_STREAM OFF\n");
        exit(-1);
    }
}
void camera_exit()
{
	//解除映射
	int i;
	for (i=0; i< BUFFER_COUNT; i++)
	{
		munmap(framebuf[i].start, framebuf[i].length);
	}
	close(fd);
}

int main()
{
	int sockfd,connfd;
    int ret;
   
	
	
	

	while(1)
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		/* 自己创建一张空卡addr */
		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(6666);  //端口
		addr.sin_addr.s_addr = inet_addr("0.0.0.0"); //0代表本机 

		printf("Waiting for connection\n");
		ret = bind(sockfd,(struct sockaddr *)&addr,sizeof(addr));      //插卡
		if(ret==-1)
		{
			printf("bind failed\n");
			return -1;
		}

		
		while(1)
		{
			listen(sockfd,32);    //开机
			connfd = accept(sockfd,NULL,NULL);    //接听
			printf("connect a client\n");
			int len;
			char buf[32];
			while(1)
			{
				
				memset(buf,0,sizeof(buf));  //清空

				ret = recv(connfd,buf,sizeof(buf),0);   
				if(ret<=0)
				{
					printf("client quit\n");
					break;
				}
				camera_init();
				camera_start();	
				sleep(1);
				store_pic(connfd);
				camera_stop();
				camera_exit();
				
			}			
		}
		close(connfd);
		close(sockfd);
			
	}
	
		
		
	
}
