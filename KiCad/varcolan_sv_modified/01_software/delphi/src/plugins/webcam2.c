#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <linux/videodev2.h>
#include "jpeglib.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "support.h"

#undef PROT_NONE
#include <sys/mman.h>

#define IMAGE	"/tmp/webcam.jpg"
#define IMAGE1	"/tmp/webcam-1.jpg"
#define IMAGE2	"/tmp/webcam-2.jpg"
#define IMAGE3	"/tmp/webcam-3.jpg"

#define VIDIOCPWCSCQUAL		_IOW('v', 195, int)

/* Già definito in support.h */
//typedef void* (*PthreadFunction)(void*);

//#define MIRROR_IMAGE

#ifdef MIRROR_IMAGE
#error
static char *yuv2;
#endif

#if 1
#define XXX 160
#define YYY 120
#define COMPR	65
#else
#define XXX 320
#define YYY 240
#define COMPR	45
#endif

static int write_file(char *data, int width, int height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *fp;
    int i, j, k;
#ifdef DEBUG
    struct timeval tv1, tv2;
#endif
/*
fp = fopen(IMAGE,"w");
fwrite(data, width*height*3/2, 1, fp);
fclose(fp);
return 0;
*/

    JSAMPROW row0[8];
    JSAMPROW row1[8];
    JSAMPROW row2[8];
    JSAMPARRAY scanarray[3] = { row0, row1, row2 };
    char tmpY[8][XXX];
    char tmpU[8][XXX/2];
    char tmpV[8][XXX/2];
    
    fp = fopen(IMAGE,"w");
    
#ifdef DEBUG
    gettimeofday(&tv1, NULL);
#endif
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, COMPR, TRUE);
    
    cinfo.dct_method = JDCT_IFAST;
    cinfo.input_gamma = 1.0;
    cinfo.raw_data_in = TRUE;
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 1;	/*1||2 */
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;	/*1||2 */
    cinfo.comp_info[2].v_samp_factor = 1;
    
    jpeg_start_compress(&cinfo, TRUE);
    
    for(i=0; i<YYY; i+=8)
    {
      for(j=0; j<8; j++)
      {
        for(k=0; k<XXX; k++)
          tmpY[j][k] = data[((i+j)*XXX*2)+(k*2)];
        for(k=0; k<XXX/2; k++)
        {
          tmpU[j][k] = data[((i+j)*XXX*2)+(k*4)+1];
          tmpV[j][k] = data[((i+j)*XXX*2)+(k*4)+3];
        }
        row0[j] = tmpY[j];
        row1[j] = tmpU[j];
        row2[j] = tmpV[j];
      }
      jpeg_write_raw_data(&cinfo, scanarray, 8);
    }
    
    jpeg_finish_compress(&(cinfo));
    jpeg_destroy_compress(&(cinfo));
    
#ifdef DEBUG
    gettimeofday(&tv2, NULL);
#endif
    
    fclose(fp);
    
#ifdef DEBUG
    tv2.tv_sec -= tv1.tv_sec;
    tv2.tv_usec -= tv1.tv_usec;
    if(tv2.tv_usec < 0)
    {
      tv2.tv_usec += 1000000;
      tv2.tv_sec--;
    }
    printf("%ld.%06ld\n", tv2.tv_sec, tv2.tv_usec);
#endif
    
    return 0;
}

#define NB_BUFFER 4
#define DHT_SIZE 432

struct vdIn {
    int fd;
    char *videodevice;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    unsigned char *tmpbuffer;
    unsigned char *framebuffer;
    int isstreaming;
    int grabmethod;
    int width;
    int height;
    int fps;
    int formatIn;
    int framesizeIn;
};

static int init_v4l2(struct vdIn *vd);

int init_videoIn(struct vdIn *vd, char *device, int width, int height, int fps,
	     int format, int grabmethod)
{
    if (vd == NULL || device == NULL)
	return -1;
    if (width == 0 || height == 0)
	return -1;
    if (grabmethod < 0 || grabmethod > 1)
	grabmethod = 1;		//mmap by default;
    vd->videodevice = NULL;
    vd->videodevice = (char *) calloc(1, 16 * sizeof(char));
    snprintf(vd->videodevice, 12, "%s", device);
    vd->isstreaming = 0;
    vd->width = width;
    vd->height = height;
    vd->fps = fps;
    vd->formatIn = format;
    vd->grabmethod = grabmethod;
    if (init_v4l2(vd) < 0) {
	printf(" Init v4L2 failed !! exit fatal \n");
	goto error;;
    }
    /* alloc a temp buffer to reconstruct the pict */
    vd->framesizeIn = (vd->width * vd->height << 1);
    switch (vd->formatIn) {
    case V4L2_PIX_FMT_MJPEG:
	vd->tmpbuffer =
	    (unsigned char *) calloc(1, (size_t) vd->framesizeIn);
	if (!vd->tmpbuffer)
	    goto error;
	vd->framebuffer =
	    (unsigned char *) calloc(1,
				     (size_t) vd->width * (vd->height +
							   8) * 2);
	break;
    case V4L2_PIX_FMT_YUYV:
	vd->framebuffer =
	    (unsigned char *) calloc(1, (size_t) vd->framesizeIn);
	break;
    default:
	printf(" should never arrive exit fatal !!\n");
	goto error;
	break;
    }
    if (!vd->framebuffer)
	goto error;
    return 0;
  error:
    free(vd->videodevice);
    close(vd->fd);
    return -1;
}

static int init_v4l2(struct vdIn *vd)
{
    int i;
    int ret = 0;

    if ((vd->fd = open(vd->videodevice, O_RDWR)) == -1) {
	perror("ERROR opening V4L interface \n");
	goto fatal;
    }
    memset(&vd->cap, 0, sizeof(struct v4l2_capability));
    ret = ioctl(vd->fd, VIDIOC_QUERYCAP, &vd->cap);
    if (ret < 0) {
	printf("Error opening device %s: unable to query device.\n",
	       vd->videodevice);
	goto fatal;
    }

    if ((vd->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
	printf("Error opening device %s: video capture not supported.\n",
	       vd->videodevice);
	goto fatal;;
    }
    if (vd->grabmethod) {
	if (!(vd->cap.capabilities & V4L2_CAP_STREAMING)) {
	    printf("%s does not support streaming i/o\n", vd->videodevice);
	    goto fatal;
	}
    } else {
	if (!(vd->cap.capabilities & V4L2_CAP_READWRITE)) {
	    printf("%s does not support read i/o\n", vd->videodevice);
	    goto fatal;
	}
    }
    /* set format in */
    memset(&vd->fmt, 0, sizeof(struct v4l2_format));
    vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->fmt.fmt.pix.width = vd->width;
    vd->fmt.fmt.pix.height = vd->height;
    vd->fmt.fmt.pix.pixelformat = vd->formatIn;
    vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(vd->fd, VIDIOC_S_FMT, &vd->fmt);
    if (ret < 0) {
	goto fatal;
    }
    if ((vd->fmt.fmt.pix.width != vd->width) ||
	(vd->fmt.fmt.pix.height != vd->height)) {
	printf(" format asked unavailable get width %d height %d \n",
	       vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
	vd->width = vd->fmt.fmt.pix.width;
	vd->height = vd->fmt.fmt.pix.height;
	/* look the format is not part of the deal ??? */
	//vd->formatIn = vd->fmt.fmt.pix.pixelformat;
    }
    
        /* set framerate */
    struct v4l2_streamparm* setfps;  
    setfps=(struct v4l2_streamparm *) calloc(1, sizeof(struct v4l2_streamparm));
    memset(setfps, 0, sizeof(struct v4l2_streamparm));
    setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps->parm.capture.timeperframe.numerator=1;
    setfps->parm.capture.timeperframe.denominator=vd->fps;
    ret = ioctl(vd->fd, VIDIOC_S_PARM, setfps); 
       
    /* request buffers */
    memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd->rb.count = NB_BUFFER;
    vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->rb.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb);
    if (ret < 0) {
	goto fatal;
    }
    /* map the buffers */
    for (i = 0; i < NB_BUFFER; i++) {
	memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
	vd->buf.index = i;
	vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->buf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(vd->fd, VIDIOC_QUERYBUF, &vd->buf);
	if (ret < 0) {
	    goto fatal;
	}
	vd->mem[i] = mmap(0 /* start anywhere */ ,
			  vd->buf.length, PROT_READ, MAP_SHARED, vd->fd,
			  vd->buf.m.offset);
	if (vd->mem[i] == MAP_FAILED) {
	    goto fatal;
	}
    }
    /* Queue the buffers. */
    for (i = 0; i < NB_BUFFER; ++i) {
	memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
	vd->buf.index = i;
	vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->buf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
	if (ret < 0) {
	    goto fatal;;
	}
    }
    return 0;
  fatal:
    return -1;

}

static int video_enable(struct vdIn *vd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(vd->fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
	return ret;
    }
    vd->isstreaming = 1;
    return 0;
}

static int video_disable(struct vdIn *vd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(vd->fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
	return ret;
    }
    vd->isstreaming = 0;
    return 0;
}

int uvcGrab(struct vdIn *vd)
{
#define HEADERFRAME1 0xaf
    int ret, len;

    if (!vd->isstreaming)
	if (video_enable(vd))
	    goto err;
    memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
    vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(vd->fd, VIDIOC_DQBUF, &vd->buf);
    if (ret < 0) {
	goto err;
    }
    
    len = vd->buf.bytesused;
    
    if (vd->buf.bytesused > vd->framesizeIn)
	    memcpy(vd->framebuffer, vd->mem[vd->buf.index],
		   (size_t) vd->framesizeIn);
    else
	    memcpy(vd->framebuffer, vd->mem[vd->buf.index],
		   (size_t) vd->buf.bytesused);
    
    ret = ioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
    if (ret < 0) {
	goto err;
    }

    return len;
  err:
    return -1;
}

void close_v4l2(struct vdIn *vd)
{
    if (vd->isstreaming)
	video_disable(vd);
    if (vd->tmpbuffer)
	free(vd->tmpbuffer);
    vd->tmpbuffer = NULL;
    free(vd->framebuffer);
    vd->framebuffer = NULL;
    free(vd->videodevice);
    vd->videodevice = NULL;
    
    close(vd->fd);
}

static int grab(struct vdIn *vd)
{
  int ret, n;
#ifdef MIRROR_IMAGE
  int x, y;
#endif
  
  for(n=0; n<5; n++)
    ret = uvcGrab(vd);
  
#ifdef MIRROR_IMAGE
#error
  for(y=0;y<YYY;y++)
    for(x=0;x<XXX;x++)
      yuv2[(y*XXX+(XXX-1-x))] = yuv1[y*XXX+x];
  for(y=0;y<YYY/2;y++)
    for(x=0;x<XXX/2;x++)
    {
      yuv2[(XXX*YYY)+(y*(XXX/2)+((XXX/2)-1-x))] = yuv1[(XXX*YYY)+y*(XXX/2)+x];
      yuv2[(XXX*YYY)+(XXX*YYY/4)+(y*(XXX/2)+((XXX/2)-1-x))] = yuv1[(XXX*YYY)+(XXX*YYY/4)+y*(XXX/2)+x];
    }  
  write_file(yuv2, XXX, YYY);
#else
  write_file(vd->framebuffer, XXX, YYY);
#endif
  
  return ret;
}

static void webcam(void *null)
{
  int ret, cam;
  char buf[24];
  struct vdIn vd;
  
  sprintf(buf, "Webcam UVC init [%d]", getpid());
  support_log(buf);
  
#ifdef MIRROR_IMAGE
#error
  yuv2 = malloc(XXX*YYY + XXX*YYY/2);
#endif
  
  while(1)
  {
  
    cam = 0;
    
    /* 25/05/2011 Invece di provare ad aprire tutti i possibili device,
       si può andare a colpo sicuro leggendo /sys/class/video4linux. */
    do
    {
      sleep(1);
      sprintf(buf, "/dev/video%d", cam++);
      ret = init_videoIn(&vd, buf, XXX, YYY, 5, V4L2_PIX_FMT_YUYV, 1);
      /* Cicla su tutte le telecamere fino a trovare quella registrata. */
      cam &= 0x03;
    }
    while(ret < 0);
  
  grab(&vd);
  
  while(1)
  {
    if(grab(&vd) < 0)
    {
      close_v4l2(&vd);
      break;
    }
    
    rename(IMAGE2, IMAGE3);
    rename(IMAGE1, IMAGE2);
    rename(IMAGE, IMAGE1);
  }
  
  }
}

static void webcam_http(void *null)
{
  int sock;
  struct sockaddr_in sa;
  static int one = 1;
  int s, i, n;
  char buf[1024];
  FILE *fp;
  
  sprintf(buf, "Webcam HTTP init [%d]", getpid());
  support_log(buf);
  
  do
  {
    sleep(1);
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock >= 0)
    {
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
      sa.sin_family = AF_INET;
      sa.sin_port = htons(80);
      sa.sin_addr.s_addr = INADDR_ANY;
      if(bind(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))
      {
        close(sock);
        continue;
      }
      listen(sock, 0);
    }
  }
  while(sock < 0);
  
  while(1)
  {
    s = accept(sock, NULL, NULL);
  
    i = 0;
    while((i < 4) || (strncmp(buf+i-4, "\r\n\r\n", 4)))
    {
      n = recv(s, buf+i, 1024-i, 0);
      i += n;
    }
  
    send(s, "HTTP/1.1 200 OK\r\n", 17, 0);
    send(s, "Refresh: 1\r\n", 12, 0);
    send(s, "Content-Type: image/jpeg\r\n\r\n", 28, 0);
  
    fp = fopen(IMAGE1, "r");
    if(fp)
    {
    while(((n = fread(buf, 1, 1024, fp)) > 0))
      send(s, buf, n, 0);
    fclose(fp);
    }
    shutdown(s, 2);
    close(s);
  }
}

void _init()
{
  pthread_t pth;
  
  printf("Webcam UVC (plugin): " __DATE__ " " __TIME__ "\n");
  
  pthread_create(&pth,  NULL, (PthreadFunction)webcam, NULL);
  pthread_detach(pth);
  pthread_create(&pth,  NULL, (PthreadFunction)webcam_http, NULL);
  pthread_detach(pth);
}
