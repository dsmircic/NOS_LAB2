/*
 * message_queue.c -- module implementation
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
#include <linux/ioctl.h>

#include "config.h"
#define THIS_MODULE  (&__this_module)

/* Buffer size */
static int buffer_size = BUFFER_SIZE;

/* Some parameters can be given at module load time */
module_param(buffer_size, int, S_IRUGO);
MODULE_PARM_DESC(buffer_size, "Buffer size in bytes, must be a power of 2");

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

struct message_queue_dev *input_dev = NULL;   /* gets data from user into in_buff */
struct message_queue_dev *output_dev = NULL;  /* gets data from in_buff to user */
struct buffer *in_buff = NULL, *out_buff = NULL;
static dev_t dev_no = 0;


/* prototypes */
static struct buffer *buffer_create(size_t, int *);
static void buffer_delete(struct buffer *);
static struct message_queue_dev *message_queue_create(dev_t, struct file_operations *,
										struct buffer *, struct buffer *, int *);
static void message_queue_delete(struct message_queue_dev *);
static void cleanup(void);
static void dump_buffer(char *prefix, struct buffer *b);

static int message_queue_open_read(struct inode *inode, struct file *filp);
static int message_queue_open_write(struct inode *inode, struct file *filp);
static ssize_t message_queue_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t message_queue_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations input_fops = {
	.owner = THIS_MODULE,
	.open = message_queue_open_write,
	.write = message_queue_write};

static struct file_operations output_fops = {
	.owner = THIS_MODULE,
	.open = message_queue_open_read,
	.read = message_queue_read};

/* init module */
static int __init message_queue_module_init(void)
{
	int retval;
	dev_t devno;

	klog(KERN_NOTICE, "Module started initialization");

	/* get device number(s) */
	retval = alloc_chrdev_region(&dev_no, 0, 3, DRIVER_NAME);
	if (retval < 0)
	{
		klog(KERN_WARNING, "Can't get major device number");
		return retval;
	}

	/* create a buffer */
	/* buffer size must be a power of 2 */
	if (!is_power_of_2(buffer_size))
		buffer_size = roundup_pow_of_two(buffer_size);
	in_buff = buffer_create(buffer_size, &retval);
	out_buff = buffer_create(buffer_size, &retval);
	if (!in_buff || !out_buff)
		goto no_driver;

	/* create devices */
	devno = dev_no;
	input_dev = message_queue_create(devno, &input_fops, in_buff, NULL, &retval);
	devno = MKDEV(MAJOR(devno), MINOR(devno) + 1);
	devno = MKDEV(MAJOR(devno), MINOR(devno) + 1);
	output_dev = message_queue_create(devno, &output_fops, NULL, out_buff, &retval);
	if (!input_dev || !output_dev)
		goto no_driver;

	/* Create timer */

	klog(KERN_NOTICE, "Module initialized with major=%d", MAJOR(devno));

	return 0;

no_driver:
	cleanup();

	return retval;
}

static void cleanup(void)
{
	if (input_dev)
		message_queue_delete(input_dev);
	if (output_dev)
		message_queue_delete(output_dev);
	if (in_buff)
		buffer_delete(in_buff);
	if (out_buff)
		buffer_delete(out_buff);
	if (dev_no)
		unregister_chrdev_region(dev_no, 3);

}

/* called when module exit */
static void __exit message_queue_module_exit(void)
{
	klog(KERN_NOTICE, "Module started exit operation");
	cleanup();
	klog(KERN_NOTICE, "Module finished exit operation");
}

module_init(message_queue_module_init);
module_exit(message_queue_module_exit);

/* Create and initialize a single buffer */
static struct buffer *buffer_create(size_t size, int *retval)
{
	struct buffer *buffer = kmalloc(sizeof(struct buffer) + size, GFP_KERNEL);
	if (!buffer)
	{
		*retval = -ENOMEM;
		klog(KERN_WARNING, "kmalloc failed\n");
		return NULL;
	}
	*retval = kfifo_init(&buffer->fifo, buffer + 1, size);
	if (*retval)
	{
		kfree(buffer);
		klog(KERN_WARNING, "kfifo_init failed\n");
		return NULL;
	}
	spin_lock_init(&buffer->key);

	*retval = 0;

	return buffer;
}
static void buffer_delete(struct buffer *buffer)
{
	kfree(buffer);
}

static void dump_buffer(char *prefix, struct buffer *b)
{
	char buf[BUFFER_SIZE];
	size_t copied;

	memset(buf, 0, BUFFER_SIZE);
	copied = kfifo_out_peek(&b->fifo, buf, BUFFER_SIZE);

	LOG("%s:size=%u:contains=%u:buf=%s", prefix,
		kfifo_size(&b->fifo), kfifo_len(&b->fifo), buf);
}

/* Create and initialize a single message_queue_dev */
static struct message_queue_dev *message_queue_create(dev_t dev_no,
										struct file_operations *fops, struct buffer *in_buff,
										struct buffer *out_buff, int *retval)
{
	struct message_queue_dev *message_queue = kmalloc(sizeof(struct message_queue_dev), GFP_KERNEL);
	if (!message_queue)
	{
		*retval = -ENOMEM;
		klog(KERN_WARNING, "kmalloc failed\n");
		return NULL;
	}
	memset(message_queue, 0, sizeof(struct message_queue_dev));
	message_queue->in_buff = in_buff;
	message_queue->out_buff = out_buff;
    message_queue->n_threads = 0;

	cdev_init(&message_queue->cdev, fops);
	message_queue->cdev.owner = THIS_MODULE;
	message_queue->cdev.ops = fops;
	*retval = cdev_add(&message_queue->cdev, dev_no, 1);
	message_queue->dev_no = dev_no;
	if (*retval)
	{
		klog(KERN_WARNING, "Error (%d) when adding device", *retval);
		kfree(message_queue);
		message_queue = NULL;
	}

	return message_queue;
}

static void message_queue_delete(struct message_queue_dev *message_queue)
{
    message_queue->n_threads--;
	cdev_del(&message_queue->cdev);
	kfree(message_queue);
}

/* open for output_dev and control_dev */
static int message_queue_open_read(struct inode *inode, struct file *filp)
{
    if (message_queue->n_threads > MAX_THREADS)
        return -EBUSY;

	struct message_queue_dev *message_queue;
    message_queue->n_threads++;

	message_queue = container_of(inode->i_cdev, struct message_queue_dev, cdev);
	filp->private_data = message_queue;

	if ((filp->f_flags & O_ACCMODE) != O_RDONLY)
		return -EPERM;

	return 0;
}

/* open for input_dev */
static int message_queue_open_write(struct inode *inode, struct file *filp)
{
	/* todo (similar to message_queue_open_read) */

    if (message_queue->n_threads > MAX_THREADS)
        return -EBUSY;

	struct message_queue_dev *message_queue;
    message_queue->n_threads++;

	message_queue = container_of(inode->i_cdev, struct message_queue_dev, cdev);
	filp->private_data = message_queue;

	if ((filp->f_flags & O_ACCMODE) != O_WRONLY)
		return -EPERM;

	return 0;
}

/* output_dev only */
static ssize_t message_queue_read(struct file *filp, char __user *ubuf, size_t count,
						   loff_t *f_pos)
{
	ssize_t retval = 0;
	struct message_queue_dev *message_queue = filp->private_data;
	struct buffer *out_buff = message_queue->out_buff;
	struct kfifo *fifo = &out_buff->fifo;
	unsigned int copied;

	spin_lock(&out_buff->key);

	dump_buffer("out_dev-end:out_buff:", out_buff);

	retval = kfifo_to_user(fifo, (char __user *)ubuf, count, &copied);
	if (retval)
		klog(KERN_WARNING, "kfifo_to_user failed\n");
	else
		retval = copied;

	dump_buffer("out_dev-end:out_buff:", out_buff);

	spin_unlock(&out_buff->key);

	return retval;
}

/* input_dev only */
static ssize_t message_queue_write(struct file *filp, const char __user *ubuf,
							size_t count, loff_t *f_pos)
{
	/* todo (similar to read) */
	ssize_t retval = 0;
	struct message_queue_dev *message_queue = filp->private_data;
	struct buffer *in_buff = message_queue->in_buff;
	struct kfifo *fifo = &in_buff->fifo;
	unsigned int copied;

	spin_lock(&in_buff->key);
	dump_buffer("in_dev-end:in_buff:", in_buff);

	// write to out_buff
	retval = kfifo_from_user(fifo, (char __user *)ubuf, count, &copied);
	if (retval == -1)
		klog(KERN_WARNING, "kfifo_from_user failed\n");
	else
		retval = copied;

	printk("Wrote: %ld\n", retval);

	dump_buffer("in_dev-end:in_buff:", in_buff);

	spin_unlock(&in_buff->key);

	return count;
}
