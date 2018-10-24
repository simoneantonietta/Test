#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef DEBUG
#include <sys/time.h>
#include <time.h>
#endif

#if defined __i386__ || defined __x86_64__
#include <libv4l1.h>
#else
#include <linux/videodev.h>
#endif
#include "jpeglib.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "support.h"

#define IMAGE	"/tmp/webcam.jpg"
#define IMAGE1	"/tmp/webcam-1.jpg"
#define IMAGE2	"/tmp/webcam-2.jpg"
#define IMAGE3	"/tmp/webcam-3.jpg"

#define VIDIOCPWCSCQUAL		_IOW('v', 195, int)

/* Già definito in support.h */
//typedef void* (*PthreadFunction)(void*);

//#define MIRROR_IMAGE

static char *yuv1;
#ifdef MIRROR_IMAGE
static char *yuv2;
#endif
static int wc_fd;

#if 0
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
    int i, j;
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
        row0[j] = data+((i+j)*XXX);
        row1[j] = data+(XXX*YYY)+((((i+j)/2)*(XXX/2)));
        row2[j] = data+(XXX*YYY)+(XXX*YYY)/4+((((i+j)/2)*(XXX/2)));
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

static int grab(int fd)
{
  int ret;
#ifdef MIRROR_IMAGE
  int x, y;
#endif
  
  ret = read(fd, yuv1, XXX*YYY + XXX*YYY/2);
  
#ifdef MIRROR_IMAGE
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
  write_file(yuv1, XXX, YYY);
#endif
  
  return ret;
}

static void webcam(void *null)
{
  int comp;
  struct video_picture vp;
  struct video_window vw;
  char buf[24];
  
  sprintf(buf, "Webcam init [%d]", getpid());
  support_log(buf);
  
  yuv1 = malloc(XXX*YYY + XXX*YYY/2);
#ifdef MIRROR_IMAGE
  yuv2 = malloc(XXX*YYY + XXX*YYY/2);
#endif
  
  do
  {
    sleep(1);
    wc_fd = open("/dev/video0", O_RDWR);
  }
  while(wc_fd < 0);
  
  ioctl(wc_fd, VIDIOCGPICT, &vp);
  
  comp = 0;
  ioctl(wc_fd, VIDIOCPWCSCQUAL, &comp);

  vw.x = 0;
  vw.y = 0;
  vw.width = XXX;
  vw.height = YYY;
  vw.flags = 5<<16;
  ioctl(wc_fd, VIDIOCSWIN, &vw);
    
  grab(wc_fd);
  
  while(1)
  {
    sleep(1);
    grab(wc_fd);
    rename(IMAGE2, IMAGE3);
    rename(IMAGE1, IMAGE2);
    rename(IMAGE, IMAGE1);
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
      return;
    }
    listen(sock, 0);
  }
  
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
    while(((n = fread(buf, 1, 1024, fp)) > 0))
      send(s, buf, n, 0);
    fclose(fp);
  
    shutdown(s, 2);
    close(s);
  }
}

void _init()
{
  FILE *fp;
  pthread_t pth;
  
  printf("Webcam (plugin): " __DATE__ " " __TIME__ "\n");
  
  fp = fopen("/proc/usbenable", "w");
  if(fp)
  {
    fprintf(fp, "1");
    fclose(fp);
  }
  
  pthread_create(&pth,  NULL, (PthreadFunction)webcam, NULL);
  pthread_detach(pth);
  pthread_create(&pth,  NULL, (PthreadFunction)webcam_http, NULL);
  pthread_detach(pth);
}
