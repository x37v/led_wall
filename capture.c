/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>
#include "led_driver.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define BLACK_THRESH 22

typedef enum {
   IO_METHOD_READ,
   IO_METHOD_MMAP,
   IO_METHOD_USERPTR,
} io_method;

struct buffer {
   void *start;
   size_t length;
};

uint8_t gamma_table[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 29, 30, 30, 31, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 39, 40, 40, 41, 42, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 49, 50, 51, 51, 52, 53, 54, 54, 55, 56, 56, 57, 58, 59, 60, 60, 61, 62, 63, 64, 64, 65, 66, 67, 68, 69, 69, 70, 71, 72, 73, 74, 75, 76, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 99, 100, 101, 102, 103, 104, 105, 106, 107, 109, 110, 111, 112, 113, 115, 116, 117, 118, 119, 121, 122, 123, 124, 126, 127
};

inline uint8_t gamma_map(int v) {
  if (v >= 256)
    return 0x7F;
  else if (v < 0)
    return 0;
  else
    return gamma_table[v];
}

static char *           dev_name        = NULL;
static char *           output_name        = NULL;
static io_method io  = IO_METHOD_MMAP;
static int fd              = -1;
struct buffer *buffers         = NULL;
static unsigned int n_buffers       = 0;

#define led_columns (8 * 12)
#define led_rows 64
const unsigned int num_leds = (led_columns * led_rows);

static int bytesperline = 0;
static int width = 720;
static int height = 480;

static uint8_t * rgb_buffer = NULL;
static uint8_t * led_buffer = NULL;

static void errno_exit(const char *s) {
   fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));

   exit (EXIT_FAILURE);
}

static int xioctl(int fd, int request, void * arg){
   int r;

   do r = ioctl (fd, request, arg);
   while (-1 == r && EINTR == errno);

   return r;
}

//totally untested
/*
static void yuyv2rgb(int y, int u, int v, int * r, int * g, int * b) {
   *r = (int)(1.164 * (float)(y - 16) + 1.596 * (float)(v - 128));
   *g = (int)(1.164 * (float)(y - 16) - 0.813 * (float)(v - 128) - 0.391 * (float)(u - 128));
   *b = (int)(1.164 * (float)(y - 16) + 2.018 * (float)(u - 128));
   if (*r > 255)
      *r = 255;
   if (*g > 255)
      *g = 255;
   if (*b > 255)
      *b = 255;
}
*/

//http://en.wikipedia.org/wiki/YCbCr#CbCr_Planes_at_different_Y_values
static void ycrcb2rgb(int y, uint8_t cr, uint8_t cb, int * r, int * g, int * b) {
   float fy = (float)y;
   float fcr = (float)cr;
   float fcb = (float)cb;

   //jpg
   //*r = (int)((float)y + 1.402f * (float)(cr - 128));
   //*g = (int)((float)y - 0.34414 * (float)(cb - 128) - 0.71414 * (float)(cr - 128));
   //*b = (int)((float)y + 1.772 * (float)(cb - 128));
   
   //*r = (int)(255.0 / 219.0 * (fy - 16.0) + (255.0 / 112.0) * 0.701 * (fcr - 128.0));
   //*g = (int)(255.0 / 219.0 * (fy - 16.0) - (255.0 / 112.0) * 0.886 * (0.114 / 0.587) * (fcb - 128.0) - (255.0 / 112.0) * 0.701 * (0.299 / 0.587) * (fcr - 128.0));
   //*b = (int)(255.0 / 219.0 * (fy - 16.0) + (255.0 / 112.0) * 0.886 * (fcb - 128.0));
   
   float u = cr;
   float v = cb;

   *b = (int)(1.164 * (y - 16) + 2.018 * (u - 128));
   *g = (int)(1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128));
   *r = (int)(1.164 * (y - 16) + 1.596 * (v - 128));

   if (*r < BLACK_THRESH && *g < BLACK_THRESH && *b < BLACK_THRESH) {
      *r = 0;
      *g = 0;
      *b = 0;
   }

   //printf("%i %i %i\n", *r, *g, *b);
}

static void process_image(struct buffer * b) {
   //struct v4l2_buffer * vbuff = (struct v4l2_buffer *)b;
   //printf("%d\n", vbuff->field);

   //printf("%d\n", (int)b->length);
   //fputc ('.', stdout);
   //fflush (stdout);
   //
   
  unsigned char *buff = b->start;

  //printf("P2\n%d %d\n255\n", width, height);
  int row, col;
  for (row=0; row<height; row++) {
    for (col=0; col < width; col++) {
       int r,g,b;
       int y, cb, cr;
       int y_off = 2 * col + (2 * row*width);

       y = *(buff + y_off);
       if (col % 2 == 0) {
          cb = buff[y_off + 1];
          cr = buff[y_off + 3];
       } else {
          cb = buff[y_off - 1];
          cr = buff[y_off + 1];
       }

#if 1
       ycrcb2rgb(y, cb, cr, &r, &g, &b);
       //ycrcb2rgb(y, cr, cb, &r, &g, &b);
       rgb_buffer[0 + 3 *(col + row * width)] = gamma_map(g);
       rgb_buffer[1 + 3 *(col + row * width)] = gamma_map(r);
       rgb_buffer[2 + 3 *(col + row * width)] = gamma_map(b);
       //rgb_buffer[0 + 3 *(col + row * width)] = 0x7F & (g >> 1);
       //rgb_buffer[1 + 3 *(col + row * width)] = 0x7F & (r >> 1);
       //rgb_buffer[2 + 3 *(col + row * width)] = 0x7F & (b >> 1);
#else
       rgb_buffer[0 + 3 *(col + row * width)] = 
          rgb_buffer[1 + 3 *(col + row * width)] = 
          rgb_buffer[2 + 3 *(col + row * width)] = 
          0x7F & (y >> 2);
#endif

      //int intensity = *(y + col);
      //printf("%d ", intensity);
    }
    //printf("\n");
    //y += bytesperline;
  }

  //XXX very rough calculations right now
  unsigned int i;
  for (i = 0; i < num_leds; i++) {
     col = ((i / led_rows) * width) / led_columns;
     if (i % 128 >= 64)
        row = i % 64;
     else
        row = 63 - (i % 64);

     row = (row * height) / 64;
     uint8_t * pixel_loc = rgb_buffer + 3 * (col + row * width);

     memcpy(led_buffer + i * 3, pixel_loc, 3 * sizeof(uint8_t));
  }

  led_write_buffer(led_buffer);
}

static int read_frame(void) {
   struct v4l2_buffer buf;
   unsigned int i;

   switch (io) {
      case IO_METHOD_READ:
         if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
            switch (errno) {
               case EAGAIN:
                  return 0;

               case EIO:
                  /* Could ignore EIO, see spec. */

                  /* fall through */

               default:
                  errno_exit ("read");
            }
         }

         process_image(buffers);

         break;

      case IO_METHOD_MMAP:
         CLEAR (buf);

         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
         buf.memory = V4L2_MEMORY_MMAP;

         if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
               case EAGAIN:
                  return 0;

               case EIO:
                  /* Could ignore EIO, see spec. */

                  /* fall through */

               default:
                  errno_exit ("VIDIOC_DQBUF");
            }
         }

         assert (buf.index < n_buffers);

         process_image(buffers + buf.index);

         if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");

         break;

      case IO_METHOD_USERPTR:
#if 0
#error "IO_METHOD_USERPTR not supported"
         CLEAR (buf);

         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
         buf.memory = V4L2_MEMORY_USERPTR;

         if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
               case EAGAIN:
                  return 0;

               case EIO:
                  /* Could ignore EIO, see spec. */

                  /* fall through */

               default:
                  errno_exit ("VIDIOC_DQBUF");
            }
         }

         for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long) buffers[i].start
                  && buf.length == buffers[i].length)
               break;

         assert (i < n_buffers);

         process_image((void *)buf.m.userptr);

         if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");
#endif

         break;
   }

   return 1;
}

static void mainloop (void) {
   unsigned int count;

#if 0
   count = 1000;
   while (count-- > 0) {
#else
   while (1) {
#endif
      for (;;) {
         fd_set fds;
         struct timeval tv;
         int r;

         FD_ZERO (&fds);
         FD_SET (fd, &fds);

         /* Timeout. */
         tv.tv_sec = 2;
         tv.tv_usec = 0;

         r = select (fd + 1, &fds, NULL, NULL, &tv);

         if (-1 == r) {
            if (EINTR == errno)
               continue;

            errno_exit ("select");
         }

         if (0 == r) {
            fprintf (stderr, "select timeout\n");
            exit (EXIT_FAILURE);
         }

         if (read_frame ())
            break;

         /* EAGAIN - continue select loop. */
      }
   }
}

static void stop_capturing (void) {
   enum v4l2_buf_type type;

   switch (io) {
      case IO_METHOD_READ:
         /* Nothing to do. */
         break;

      case IO_METHOD_MMAP:
      case IO_METHOD_USERPTR:
         type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

         if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
            errno_exit ("VIDIOC_STREAMOFF");

         break;
   }
}

static void start_capturing(void) {
   unsigned int i;
   enum v4l2_buf_type type;

   switch (io) {
      case IO_METHOD_READ:
         /* Nothing to do. */
         break;

      case IO_METHOD_MMAP:
         for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = i;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
               errno_exit ("VIDIOC_QBUF");
         }

         type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

         if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

         break;

      case IO_METHOD_USERPTR:
         for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_USERPTR;
            buf.index       = i;
            buf.m.userptr = (unsigned long) buffers[i].start;
            buf.length      = buffers[i].length;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
               errno_exit ("VIDIOC_QBUF");
         }

         type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

         if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

         break;
   }
}

static void uninit_device(void){
   unsigned int i;

   switch (io) {
      case IO_METHOD_READ:
         free (buffers[0].start);
         break;

      case IO_METHOD_MMAP:
         for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap (buffers[i].start, buffers[i].length))
               errno_exit ("munmap");
         break;

      case IO_METHOD_USERPTR:
         for (i = 0; i < n_buffers; ++i)
            free (buffers[i].start);
         break;
   }

   free (buffers);
}

static void init_read(unsigned int buffer_size) {
   buffers = calloc (1, sizeof (*buffers));

   if (!buffers) {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
   }

   buffers[0].length = buffer_size;
   buffers[0].start = malloc (buffer_size);

   if (!buffers[0].start) {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
   }
}

static void init_mmap(void){
   struct v4l2_requestbuffers req;

   CLEAR (req);

   req.count               = 4;
   req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   req.memory              = V4L2_MEMORY_MMAP;

   if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
      if (EINVAL == errno) {
         fprintf (stderr, "%s does not support "
               "memory mapping\n", dev_name);
         exit (EXIT_FAILURE);
      } else {
         errno_exit ("VIDIOC_REQBUFS");
      }
   }

   if (req.count < 2) {
      fprintf (stderr, "Insufficient buffer memory on %s\n",
            dev_name);
      exit (EXIT_FAILURE);
   }

   buffers = calloc (req.count, sizeof (*buffers));

   if (!buffers) {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
   }

   for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
      struct v4l2_buffer buf;

      CLEAR (buf);

      buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory      = V4L2_MEMORY_MMAP;
      buf.index       = n_buffers;

      if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
         errno_exit ("VIDIOC_QUERYBUF");

      buffers[n_buffers].length = buf.length;
      buffers[n_buffers].start =
         mmap (NULL /* start anywhere */,
               buf.length,
               PROT_READ | PROT_WRITE /* required */,
               MAP_SHARED /* recommended */,
               fd, buf.m.offset);

      if (MAP_FAILED == buffers[n_buffers].start)
         errno_exit ("mmap");
   }
}

static void init_userp   (unsigned int buffer_size) {
   struct v4l2_requestbuffers req;
   unsigned int page_size;

   page_size = getpagesize ();
   buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

   CLEAR (req);

   req.count               = 4;
   req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   req.memory              = V4L2_MEMORY_USERPTR;

   if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
      if (EINVAL == errno) {
         fprintf (stderr, "%s does not support "
               "user pointer i/o\n", dev_name);
         exit (EXIT_FAILURE);
      } else {
         errno_exit ("VIDIOC_REQBUFS");
      }
   }

   buffers = calloc (4, sizeof (*buffers));

   if (!buffers) {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
   }

   for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
      buffers[n_buffers].length = buffer_size;
      buffers[n_buffers].start = memalign (/* boundary */ page_size,
            buffer_size);

      if (!buffers[n_buffers].start) {
         fprintf (stderr, "Out of memory\n");
         exit (EXIT_FAILURE);
      }
   }
}

static void init_device                     (void) {
   struct v4l2_capability cap;
   struct v4l2_cropcap cropcap;
   struct v4l2_crop crop;
   struct v4l2_format fmt;
   unsigned int min;

   if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
      if (EINVAL == errno) {
         fprintf (stderr, "%s is no V4L2 device\n",
               dev_name);
         exit (EXIT_FAILURE);
      } else {
         errno_exit ("VIDIOC_QUERYCAP");
      }
   }

   if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
      fprintf (stderr, "%s is no video capture device\n",
            dev_name);
      exit (EXIT_FAILURE);
   }

   switch (io) {
      case IO_METHOD_READ:
         if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf (stderr, "%s does not support read i/o\n",
                  dev_name);
            exit (EXIT_FAILURE);
         }

         break;

      case IO_METHOD_MMAP:
      case IO_METHOD_USERPTR:
         if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf (stderr, "%s does not support streaming i/o\n",
                  dev_name);
            exit (EXIT_FAILURE);
         }

         break;
   }


   /* Select video input, video standard and tune here. */


   CLEAR (cropcap);

   cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

   if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect; /* reset to default */

      if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
         switch (errno) {
            case EINVAL:
               /* Cropping not supported. */
               break;
            default:
               /* Errors ignored. */
               break;
         }
      }
   } else { 
      /* Errors ignored. */
   }


   CLEAR (fmt);

   fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   fmt.fmt.pix.width       = width; 
   fmt.fmt.pix.height      = height;
   fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
   //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
   fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

   if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
      errno_exit ("VIDIOC_S_FMT");

   /* Note VIDIOC_S_FMT may change width and height. */

   /* Buggy driver paranoia. */
   min = fmt.fmt.pix.width * 2;
   if (fmt.fmt.pix.bytesperline < min)
      fmt.fmt.pix.bytesperline = min;
   min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
   if (fmt.fmt.pix.sizeimage < min)
      fmt.fmt.pix.sizeimage = min;

   width = fmt.fmt.pix.width;
   height = fmt.fmt.pix.height;
   bytesperline = fmt.fmt.pix.bytesperline;

   rgb_buffer = malloc(sizeof(uint8_t) * width * height * 3);
   led_buffer = malloc(sizeof(uint8_t) * num_leds * 3);

   //printf("size %d x %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
   //printf("bytes per line %d\n", fmt.fmt.pix.bytesperline);
   //printf("sizeimage %d\n", fmt.fmt.pix.sizeimage);
   //exit(0);

   switch (io) {
      case IO_METHOD_READ:
         init_read (fmt.fmt.pix.sizeimage);
         break;

      case IO_METHOD_MMAP:
         init_mmap ();
         break;

      case IO_METHOD_USERPTR:
         init_userp (fmt.fmt.pix.sizeimage);
         break;
   }
}

static void close_device                    (void) {
   if (-1 == close (fd))
      errno_exit ("close");

   fd = -1;
}

static void open_device                     (void) {
   struct stat st; 

   if (-1 == stat (dev_name, &st)) {
      fprintf (stderr, "Cannot identify '%s': %d, %s\n",
            dev_name, errno, strerror (errno));
      exit (EXIT_FAILURE);
   }

   if (!S_ISCHR (st.st_mode)) {
      fprintf (stderr, "%s is no device\n", dev_name);
      exit (EXIT_FAILURE);
   }

   fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

   if (-1 == fd) {
      fprintf (stderr, "Cannot open '%s': %d, %s\n",
            dev_name, errno, strerror (errno));
      exit (EXIT_FAILURE);
   }
}

static void usage (FILE * fp, int argc, char ** argv) {
   fprintf (fp,
         "Usage: %s [options]\n\n"
         "Options:\n"
         "-d | --device name Video device name [/dev/video]\n"
         "-h | --help Print this message\n"
         "-m | --mmap Use memory mapped buffers\n"
         "-r | --read Use read() calls\n"
         "-u | --userp Use application allocated buffers\n"
         "",
         argv[0]);
}

static const char short_options [] = "d:hmru";

static const struct option
long_options [] = {
   { "device",     required_argument,      NULL,           'd' },
   { "help",       no_argument,            NULL,           'h' },
   { "mmap",       no_argument,            NULL,           'm' },
   { "read",       no_argument,            NULL,           'r' },
   { "userp",      no_argument,            NULL,           'u' },
   { 0, 0, 0, 0 }
};

int main (int argc, char ** argv) {
   dev_name = "/dev/video0";
   output_name = "/dev/ttyUSB000";

   //printf("%ld\n", sizeof(struct v4l2_buffer));

   for (;;) {
      int index;
      int c;

      c = getopt_long (argc, argv,
            short_options, long_options,
            &index);

      if (-1 == c)
         break;

      switch (c) {
         case 0: /* getopt_long() flag */
            break;

         case 'd':
            dev_name = optarg;
            break;

         case 'o':
            output_name = optarg;

         case 'h':
            usage (stdout, argc, argv);
            exit (EXIT_SUCCESS);

         case 'm':
            io = IO_METHOD_MMAP;
            break;

         case 'r':
            io = IO_METHOD_READ;
            break;

         case 'u':
            io = IO_METHOD_USERPTR;
            break;

         default:
            usage (stderr, argc, argv);
            exit (EXIT_FAILURE);
      }
   }

   if (!led_open_output(output_name, num_leds)) {
      printf("cannot open output %s\n", output_name);
      exit (EXIT_FAILURE);
   }

   open_device ();
   init_device ();
   start_capturing ();
   mainloop ();
   stop_capturing ();
   uninit_device ();
   close_device ();
   exit (EXIT_SUCCESS);
   return 0;
}
