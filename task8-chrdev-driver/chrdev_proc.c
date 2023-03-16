#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kriz");
MODULE_DESCRIPTION("Simple character device driver witn procfs");
MODULE_VERSION("0.1");

#define CLASS_NAME  	"chrdev"
#define DEVICE_NAME 	"chrdev_simple"
#define BUFFER_SIZE 	 1024
#define PROC_BUFFER_SIZE 500
#define PROC_FILE_NAME   "dummy"
#define PROC_DIR_NAME    "chrdev"


static int is_open;
static int data_size;
static unsigned char data_buffer[BUFFER_SIZE];
static char procfs_buffer[PROC_BUFFER_SIZE] = {0};
static size_t procfs_buffer_size = 0;

static struct class *pclass;
static struct device *pdev;
static struct cdev chrdev_cdev; 
static struct proc_dir_entry *proc_file, *proc_folder;
dev_t dev = 0;					


static ssize_t proc_chrdev_read(struct file *filep, char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t to_copy, not_copied, delta;
	
	if(0 == procfs_buffer_size)
	{
		return 0;
	}
	
	to_copy = min(count, procfs_buffer_size);

	not_copied = copy_to_user(buffer, procfs_buffer, to_copy);

	delta = to_copy - not_copied;
	procfs_buffer_size =- delta;


	return delta;
}

static struct proc_ops proc_fops = {
	.proc_read = proc_chrdev_read
};

static int dev_open (struct inode *inodep, struct file *filep)
{
	if (is_open) {
		pr_err("chrdev: already open\n");
		return -EBUSY;
	}

	is_open = 1;
	pr_info("chrdev: device opened\n");
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	is_open = 0;
	pr_info("chrdev: device closed\n");
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int ret;

	pr_info("chrdev: read from file %s\n", filep->f_path.dentry->d_iname);//-------------------------------
	pr_info("chrdev: read from device %d:%d\n", imajor(filep->f_inode), iminor(filep->f_inode));

	if (len > data_size) len = data_size; 

	
	if (copy_to_user(buffer, data_buffer, len)) {
		pr_err("chrdev: copy_to_user failed\n");
		return -EIO;
	}
	data_size = 0;
	pr_info("chrdev: %zu bytes read\n", len);

	sprintf(procfs_buffer, "Module name: %s\n", DEVICE_NAME);
	sprintf(procfs_buffer + 100, "CharDev Read/Write buffer size: %d\n", BUFFER_SIZE);
	//----------------------------------------------------------------------------------------------------
	return len;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	int ret;

	pr_info("chrdev: write to file %s\n", filep->f_path.dentry->d_iname);
	pr_info("chrdev: write to device %d:%d\n", imajor(filep->f_inode), iminor(filep->f_inode));

	data_size = len;
	if(data_size > BUFFER_SIZE) data_size = BUFFER_SIZE;

	ret = copy_from_user(data_buffer, buffer, data_size);
	if(ret) {
		pr_err("chrdev: copy_from_user failed: %d\n", ret);
		return -EFAULT;
	}

	pr_info("chrdev: %d bytes written\n", data_size);
	return data_size;
}

/*Callbacks provided by the driver*/
static struct file_operations chrdev_fops =
{
	.owner = THIS_MODULE,//?
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
};

static int __init chrdev_init(void) 
{
	/* create procfs entry*/
	proc_folder = proc_mkdir(PROC_DIR_NAME, NULL);
	if(IS_ERR(proc_folder))
	{
		pr_err("%s: %s\n", DEVICE_NAME, "Error: COULD not create proc_fs folder");
		goto procfs_folder_err;
	}

	proc_file = proc_create(PROC_FILE_NAME, 0666, proc_folder, &proc_fops);
	if(IS_ERR(proc_file))
	{
		pr_err("%s: %s\n", DEVICE_NAME, "Error: Could not initialize proc_fs file");
		goto procfs_file_err;
	}

	is_open = 0;
	data_size = 0;

	/*Define major and minor*/
	if (alloc_chrdev_region (&dev, 3, 1, DEVICE_NAME)) {
		pr_err("%s: %s\n", DEVICE_NAME, "Failed to alloc_chrdev_region");
		return -1;
	}

	/*Add device to the system*/
	cdev_init(&chrdev_cdev, &chrdev_fops);
	if((cdev_add(&chrdev_cdev, dev, 1)) < 0) {
		pr_err("%s: %s\n", DEVICE_NAME, "Failed to add the device to the system");
		goto cdev_err;
	}

	/*Create device class*/
	pclass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(pclass)) {
		goto class_err;
	}

	/*Create device node*/
	pdev = device_create(pclass, NULL, dev, NULL, CLASS_NAME"0");
	if (IS_ERR(pdev)) {
		goto device_err;
	}
	pr_info("%s: major = %d, minor = %d\n", DEVICE_NAME, MAJOR(dev), MINOR(dev));
	pr_info("%s: %s\n", DEVICE_NAME, "INIT");

	return 0;

procfs_file_err:
	proc_remove(proc_file);
procfs_folder_err:
	proc_remove(proc_folder);
device_err:
	class_destroy(pclass);
class_err:
	cdev_del(&chrdev_cdev);
cdev_err:
	unregister_chrdev_region(dev,1);
	return -1;
}

static void __exit chrdev_exit(void)
{
	proc_remove(proc_file);
	proc_remove(proc_folder);
	pr_info("%s: %s\n", DEVICE_NAME, "procfs entry removed");
	device_destroy(pclass, dev);
	class_destroy(pclass);
	cdev_del(&chrdev_cdev);
	unregister_chrdev_region(dev,1);
	pr_info("%s: %s\n", DEVICE_NAME, "EXIT");
}

module_init(chrdev_init);
module_exit(chrdev_exit);