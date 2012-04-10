/*
 *  V4L2 video capture example
 *
 *  http://credentiality2.blogspot.com/2010/04/v4l2-example.html
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>        /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>    /* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
   void *start;
   size_t length;
};

static int fd = -1;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;

static void errno_exit(const char *s) {
   fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));

   exit (EXIT_FAILURE);
}

static int xioctl(int fd, int request, void * arg) {
   int r;

   do r = ioctl (fd, request, arg);
   while (-1 == r && EINTR == errno);

   return r;
}

static void process_image(const void *p) {
   fputc ('.', stdout);
   fflush (stdout);
}

static int read_frame(void) {
   struct v4l2_buffer buf;
   unsigned int i;

   printf("read %d %ld %d\n", fd, (long int)buffers[0].start, (int)buffers[0].length);

   if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
      switch (errno) {
         case EAGAIN:
            return 0;

         case  EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */

         default:
            errno_exit ("read");
      }
   }

   process_image (buffers[0].start);

   return 1;
}

static void mainloop(void) {
   unsigned int count;

   count = 100;

   while (count-- > 0) {
      for (;;) {
         fd_set fds;
         struct timeval tv;
         int r;

         FD_ZERO (&fds);
         FD_SET (fd, &fds);

         /* Timeout. */
         tv.tv_sec = 2;
         tv.tv_usec = 0;

         r = select(fd + 1, &fds, NULL, NULL, &tv);

         if (-1 == r) {
            if (EINTR == errno)
               continue;

            errno_exit ("select");
         }

         if (0 == r) {
            fprintf (stderr, "select timeout\n");
            exit (EXIT_FAILURE);
         }

         if (read_frame())
            break;

         /* EAGAIN - continue select loop. */
      }
   }
}

static void uninit_device(void) {
   unsigned int i;

   free (buffers[0].start);
   free (buffers);
}

static void init_read(unsigned int buffer_size) {
   buffers = (buffer*)calloc(1, sizeof(*buffers));

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

static void init_device(char *dev_name) {
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

   if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
      fprintf (stderr, "%s does not support read i/o\n",
            dev_name);
      exit (EXIT_FAILURE);
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

   fmt.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   fmt.fmt.pix.width       = 640; 
   fmt.fmt.pix.height      = 480;

   // This worked with my capture card, but bombed with
   // "VIDIOC_S_FMT error 22, Invalid argument" on my Logitech QuickCam Pro 4000
   // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
   // This worked on the logitech:
   fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

   fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

   if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
      errno_exit("VIDIOC_S_FMT");

   /* Note VIDIOC_S_FMT may change width and height. */

   /* Buggy driver paranoia. */
   min = fmt.fmt.pix.width * 2;
   if (fmt.fmt.pix.bytesperline < min)
      fmt.fmt.pix.bytesperline = min;
   min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
   if (fmt.fmt.pix.sizeimage < min)
      fmt.fmt.pix.sizeimage = min;

   init_read (fmt.fmt.pix.sizeimage);
}

static void close_device(void) {
   if (-1 == close (fd))
      errno_exit ("close");

   fd = -1;
}

static void open_device(char *dev_name) {
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

int main(int argc, char **argv) {
   char *dev_name = "/dev/video1";

   open_device(dev_name);

   init_device(dev_name);
   mainloop();
   uninit_device();
   close_device();

   exit(EXIT_SUCCESS);
   return 0;
}
