/*
gcc -Wall -O2 svv.c -o svv $(pkg-config gtk+-2.0 libv4lconvert --cflags --libs)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>

#include <gtk/gtk.h>

#include "libv4lconvert.h"
static struct v4lconvert_data *v4lconvert_data;
static struct v4l2_format src_fmt;	/* raw format */
static unsigned char *dst_buf;

#define IO_METHOD_READ 7	/* !! must be != V4L2_MEMORY_MMAP / USERPTR */

static struct v4l2_format fmt;		/* gtk pormat */

struct buffer {
	void *start;
	size_t length;
};

static char *dev_name = "/dev/video0";
static int io = V4L2_MEMORY_MMAP;
static int fd = -1;
static struct buffer *buffers;
static int n_buffers;
static int grab, info, display_time;
#define NFRAMES 30
static struct timeval cur_time;

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	fprintf(stderr, "%s\n",
			v4lconvert_get_error_message(v4lconvert_data));
	exit(EXIT_FAILURE);
}

static int read_frame(void);

/* graphic functions */
static GtkWidget *drawing_area;

static void delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
}

static void key_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	unsigned int k;

	k = event->key.keyval;
	if (k < 0xff) {
		switch (k) {
		case 'g':
			/* grab a raw image */
			grab = 1;
			break;
		case 'i':
			info = !info;
			if (info) {
				if (io != V4L2_MEMORY_MMAP)
					info = NFRAMES;
				gettimeofday(&cur_time, 0);
			}
			break;
		case 't':
			display_time = 1;
			break;
		default:
			gtk_main_quit();
			break;
		}
	}
}

static void frame_ready(gpointer data, gint source, GdkInputCondition condition)
{
	if (condition != GDK_INPUT_READ)
		errno_exit("poll");
		
	read_frame();
}

static void set_ctrl(GtkWidget *widget, gpointer data)
{
	struct v4l2_control ctrl;
	long id;

	id = (long) data;
	ctrl.id = id;
	ctrl.value = gtk_range_get_value(GTK_RANGE(widget));
	if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0)
		fprintf(stderr, "set control error %d, %s\n",
				errno, strerror(errno));
}

static void toggle_ctrl(GtkWidget *widget, gpointer data)
{
	struct v4l2_control ctrl;
	long id;

	id = (long) data;
	ctrl.id = id;
	ctrl.value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0)
		fprintf(stderr, "set control error %d, %s\n",
				errno, strerror(errno));
}

static void set_qual(GtkWidget *widget, gpointer data)
{
	struct v4l2_jpegcompression jc;

	memset(&jc, 0, sizeof jc);
	jc.quality = gtk_range_get_value(GTK_RANGE(widget));
	if (ioctl(fd, VIDIOC_S_JPEGCOMP, &jc) < 0) {
		fprintf(stderr, "set JPEG quality error %d, %s\n",
				errno, strerror(errno));
		gtk_widget_set_state(widget, GTK_STATE_INSENSITIVE);
	}
}

static void set_parm(GtkWidget *widget, gpointer data)
{
	struct v4l2_streamparm parm;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator =
			gtk_range_get_value(GTK_RANGE(widget));
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		fprintf(stderr, "set frame rate error %d, %s\n",
				errno, strerror(errno));
		gtk_widget_set_state(widget, GTK_STATE_INSENSITIVE);
	}
	ioctl(fd, VIDIOC_G_PARM, &parm);
	gtk_range_set_value(GTK_RANGE(widget),
			parm.parm.capture.timeperframe.denominator);
}

static void reset_ctrl(GtkButton *b, gpointer data)
{
	struct v4l2_queryctrl qctrl;
	struct v4l2_control ctrl;
	long id;
	GtkWidget *val;

	id = (long) data;
	qctrl.id = ctrl.id = id;
	if (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) != 0) {
		fprintf(stderr, "query control error %d, %s\n",
				errno, strerror(errno));
		return;
	}
	ctrl.value = qctrl.default_value;
	if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0)
		fprintf(stderr, "set control error %d, %s\n",
				errno, strerror(errno));
	val = (GtkWidget *) gtk_container_get_children(
		GTK_CONTAINER(
			gtk_widget_get_parent(
				GTK_WIDGET(b))))->next->data;
	gtk_range_set_value(GTK_RANGE(val), ctrl.value);
}

static void ctrl_create(GtkWidget *vbox)
{
	GtkWidget *hbox, *c_lab, *c_val, *c_reset;
	
	struct v4l2_queryctrl qctrl = { V4L2_CTRL_FLAG_NEXT_CTRL };
	struct v4l2_control ctrl;
	struct v4l2_jpegcompression jc;
	struct v4l2_streamparm parm;
	
	long id;
	int setval;

	while (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

		c_lab = gtk_label_new(strdup((char *) qctrl.name));
		gtk_widget_set_size_request(GTK_WIDGET(c_lab), 160, -1);
		gtk_box_pack_start(GTK_BOX(hbox), c_lab, FALSE, FALSE, 0);

		setval = 1;
		switch (qctrl.type) {
		case V4L2_CTRL_TYPE_INTEGER:
		case V4L2_CTRL_TYPE_MENU:
			c_val = gtk_hscale_new_with_range(qctrl.minimum, qctrl.maximum, qctrl.step);
			gtk_scale_set_value_pos(GTK_SCALE(c_val), GTK_POS_LEFT);
			gtk_scale_set_draw_value(GTK_SCALE(c_val), TRUE);
			gtk_scale_set_digits(GTK_SCALE(c_val), 0);
			gtk_range_set_update_policy(GTK_RANGE(c_val), GTK_UPDATE_DELAYED);
			break;
		case V4L2_CTRL_TYPE_BOOLEAN:
			c_val = gtk_check_button_new();
			break;
		default:
			c_val = gtk_label_new(strdup("(not treated yet"));
			setval = 0;
			break;
		}
		
		gtk_widget_set_size_request(GTK_WIDGET(c_val), 300, -1);
		gtk_box_pack_start(GTK_BOX(hbox), c_val, FALSE, FALSE, 0);

		if (setval) {
			id = qctrl.id;
			ctrl.id = id;
			if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0)
				fprintf(stderr, "get control error %d, %s\n", errno, strerror(errno));
				
			switch (qctrl.type) {
				case V4L2_CTRL_TYPE_INTEGER:
				case V4L2_CTRL_TYPE_MENU:
					gtk_range_set_value(GTK_RANGE(c_val), ctrl.value);
					g_signal_connect(G_OBJECT(c_val), "value_changed",
						G_CALLBACK(set_ctrl), (gpointer) id);
					break;
				case V4L2_CTRL_TYPE_BOOLEAN:
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_val),
									ctrl.value);
					g_signal_connect(G_OBJECT(c_val), "toggled",
						G_CALLBACK(toggle_ctrl), (gpointer) id);
					break;
				default:
					break;
			}
			
			c_reset = gtk_button_new_with_label("Reset");
			
			gtk_box_pack_start(GTK_BOX(hbox), c_reset, FALSE, FALSE, 40);
			
			gtk_widget_set_size_request(GTK_WIDGET(c_reset), 60, 20);
			
			g_signal_connect(G_OBJECT(c_reset), "released", G_CALLBACK(reset_ctrl), (gpointer) id);

			if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
				gtk_widget_set_sensitive(c_val, FALSE);
				gtk_widget_set_sensitive(c_reset, FALSE);
			}
		}
		qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}

	/* JPEG quality */
	if (ioctl(fd, VIDIOC_G_JPEGCOMP, &jc) >= 0) {
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		c_lab = gtk_label_new("JPEG quality");
		gtk_widget_set_size_request(GTK_WIDGET(c_lab), 150, -1);
		gtk_box_pack_start(GTK_BOX(hbox), c_lab, FALSE, FALSE, 0);

		c_val = gtk_hscale_new_with_range(20, 99, 1);
		gtk_scale_set_draw_value(GTK_SCALE(c_val), TRUE);
		gtk_scale_set_digits(GTK_SCALE(c_val), 0);
		gtk_widget_set_size_request(GTK_WIDGET(c_val), 300, -1);
		gtk_range_set_value(GTK_RANGE(c_val), jc.quality);
		gtk_box_pack_start(GTK_BOX(hbox), c_val, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(c_val), "value_changed", G_CALLBACK(set_qual), (gpointer) NULL);
	}

	/* framerate */
	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_PARM, &parm) >= 0) {
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		c_lab = gtk_label_new("Frame rate");
		gtk_widget_set_size_request(GTK_WIDGET(c_lab), 150, -1);
		gtk_box_pack_start(GTK_BOX(hbox), c_lab, FALSE, FALSE, 0);
		c_val = gtk_hscale_new_with_range(5, 120, 1);
		gtk_scale_set_draw_value(GTK_SCALE(c_val), TRUE);
		gtk_scale_set_digits(GTK_SCALE(c_val), 0);
		gtk_widget_set_size_request(GTK_WIDGET(c_val), 300, -1);
		gtk_range_set_value(GTK_RANGE(c_val), parm.parm.capture.timeperframe.denominator);
		gtk_box_pack_start(GTK_BOX(hbox), c_val, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(c_val), "value_changed", G_CALLBACK(set_parm), (gpointer) NULL);
	}
}

static void main_frontend(int argc, char *argv[])
{
	GtkWidget *window, *vbox;

	gtk_init(&argc, &argv);
	gdk_rgb_init();

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title(GTK_WINDOW(window), "RoboSitter");

	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "key_release_event", GTK_SIGNAL_FUNC(key_event), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 2);

	/* vertical box */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* image */
	drawing_area = gtk_drawing_area_new();
	
	gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), fmt.fmt.pix.width, fmt.fmt.pix.height);
	
	gtk_box_pack_start(GTK_BOX(vbox), drawing_area, FALSE, FALSE, 0);

	ctrl_create(vbox);

	gtk_widget_show_all(window);

	gdk_input_add(fd, GDK_INPUT_READ, frame_ready, NULL);

	printf("'g' grab an image - 'i' info toggle - 'q' quit\n");

	gtk_main();
}

static int xioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (r < 0 && EINTR == errno);
	
	return r;
}

static void process_image(unsigned char *p, int len)
{
	if (grab) {
		FILE *f;

		f = fopen("image.dat", "w");
		fwrite(p, 1, len, f);
		fclose(f);
		printf("image dumped to 'image.dat'\n");
		//exit(EXIT_SUCCESS);
	}
	
	if (v4lconvert_convert(v4lconvert_data, &src_fmt, &fmt, p, len, dst_buf, fmt.fmt.pix.sizeimage) < 0) {
		if (errno != EAGAIN)
			errno_exit("v4l_convert");
		return;
	}
	
	p = dst_buf;
	
	len = fmt.fmt.pix.sizeimage;
	
	gdk_draw_rgb_image(drawing_area->window, drawing_area->style->white_gc, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height, GDK_RGB_DITHER_NORMAL, p, fmt.fmt.pix.width * 3);
}

static int read_frame(void)
{
	struct v4l2_buffer buf;
	int i;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	
	assert(buf.index < n_buffers);

	process_image(buffers[buf.index].start, buf.bytesused);
	
	return 1;
}

static int get_frame()
{
	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);
	
	if (r < 0) {
		if (EINTR == errno)
			return 0;

		errno_exit("select");
	}

	if (r == 0) {
		fprintf(stderr, "select timeout\n");
		exit(EXIT_FAILURE);
	}
	
	return read_frame();
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
		errno_exit("VIDIOC_STREAMOFF");
}

static void start_capturing(void)
{
	int i;
	enum v4l2_buf_type type;

	printf("mmap method\n");
	
	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		memset(&(buf), 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
			errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
		errno_exit("VIDIOC_STREAMON");
}

static void uninit_device(void)
{
	int i;
	
	for (i = 0; i < n_buffers; ++i)
		if (-1 ==
			munmap(buffers[i].start, buffers[i].length))
			errno_exit("munmap");

	free(buffers);
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	memset(&(req), 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		memset(&(buf), 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

static void init_device(int w, int h) {

	struct v4l2_capability cap;
	int ret;
	int sizeimage;

	if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}
	
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
		exit(EXIT_FAILURE);
	}

	memset(&(fmt), 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	v4lconvert_data = v4lconvert_create(fd);
	
	if (v4lconvert_data == NULL)
		errno_exit("v4lconvert_create");
		
	if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
		errno_exit("v4lconvert_try_format");
		
	ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
	sizeimage = src_fmt.fmt.pix.sizeimage;
	dst_buf = malloc(fmt.fmt.pix.sizeimage);
	
	printf("raw pixfmt: %c%c%c%c %dx%d\n", src_fmt.fmt.pix.pixelformat & 0xff, (src_fmt.fmt.pix.pixelformat >> 8) & 0xff, (src_fmt.fmt.pix.pixelformat >> 16) & 0xff, (src_fmt.fmt.pix.pixelformat >> 24) & 0xff, src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);

	if (ret < 0)
		errno_exit("VIDIOC_S_FMT");

	printf("pixfmt: %c%c%c%c %dx%d\n", fmt.fmt.pix.pixelformat & 0xff, (fmt.fmt.pix.pixelformat >> 8) & 0xff, (fmt.fmt.pix.pixelformat >> 16) & 0xff, (fmt.fmt.pix.pixelformat >> 24) & 0xff, fmt.fmt.pix.width, fmt.fmt.pix.height);

	init_mmap();
}

static void close_device(void) {

	close(fd);
}

static int open_device(void) {

	struct stat st;

	if (stat(dev_name, &st) < 0) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
	
	if (fd < 0) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd;
}

static const char short_options[] = "d:f:ghim:r";

static const struct option long_options[] = {
	{"device", required_argument, NULL, 'd'},
	{"format", required_argument, NULL, 'f'},
	{"grab", no_argument, NULL, 'g'},
	{"help", no_argument, NULL, 'h'},
	{"info", no_argument, NULL, 'i'},
	{"method", required_argument, NULL, 'm'},
	{}
};

int main(int argc, char **argv) {
	int w, h;

	w = 640;
	h = 480;

	dev_name = "/dev/video1";	
	
	open_device();
	
	init_device(w, h);
	
	start_capturing();
	
	if (grab)
		get_frame();
	else
		main_frontend(argc, argv);
		
	stop_capturing();
	
	uninit_device();
	
	close_device();
	
	return 0;
}
