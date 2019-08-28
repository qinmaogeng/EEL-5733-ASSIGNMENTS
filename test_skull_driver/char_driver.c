#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/ioctl.h>

#define CDRV_IOC_MAGIC 'k'
#define NUM 1
#define ASP_CLEAR_BUFF _IO(CDRV_IOC_MAGIC, NUM) 
#define MYDEV_NAME "mycdrv"

static size_t ramdisk_size = 16 * PAGE_SIZE; 
static unsigned int count = 1;
static struct class *foo_class;
static int NUM_DEVICES = 1; 
module_param(NUM_DEVICES, int, S_IRUGO);

struct chardevice {
	struct semaphore sem; 
	struct cdev my_cdev;
	char *ramdisk;
	int device_num;
	size_t size;
} *devices;


static int mycdrv_open(struct inode *inode, struct file *file) { 
	struct chardevice *device; 

	pr_info(" attempting to open device: %s:\n", MYDEV_NAME);
	pr_info(" MAJOR number = %d, MINOR number = %d\n", imajor(inode), iminor(inode));
	
	device = container_of(inode->i_cdev, struct chardevice, my_cdev);
	file->private_data = device; 

	pr_info("Successfully opened device: %s%d", MYDEV_NAME, device->device_num);

	return 0;
}

static int mycdrv_release(struct inode *inode, struct file *file) {
	struct chardevice *device= file->private_data;
	pr_info(" CLOSING device: %s%d", MYDEV_NAME,device->device_num );
	return 0;
}

static ssize_t mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos) {
	int nbytes, max, Bytes;

	struct chardevice* device = file->private_data;
	
	if(down_interruptible(&(device->sem))!=0){ 
		printk(KERN_ALERT "mycdrv: could not lock device during read");
		return -1;
	}

	max = device->size - *ppos;
	Bytes = max > lbuf ? lbuf : max;
	if (Bytes == 0)
		pr_info("Reached end of the device on a read");
	nbytes = Bytes - copy_to_user(buf, (device->ramdisk) + *ppos, Bytes); 
	*ppos += nbytes;
	pr_info("\n Leaving the READ function, nbytes=%d, pos=%d\n",nbytes, (int)*ppos);
	
	up(&(device->sem));

	return nbytes;
}

static ssize_t mycdrv_write(struct file *file, const char __user * buf, size_t lbuf, loff_t * ppos) {
	int nbytes, max, Bytes;

	struct chardevice* device = file->private_data;

	if(down_interruptible(&(device->sem)) != 0){
		printk(KERN_ALERT "mycdrv: could not lock device during write");
		return -1;
	}

	max = device->size - *ppos; 
	pr_info("max: %d, ramdisk size: %d, ppos:%d lbuf: %d", max,(int) device->size,(int) *ppos, (int) lbuf);
	Bytes = max > lbuf ? lbuf : max;
	if (Bytes == 0)
		pr_info("Reached end of the device on a write");
	nbytes = Bytes - copy_from_user((device->ramdisk) + *ppos, buf, Bytes); 
	*ppos += nbytes;
	pr_info("\n Leaving the WRITE function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	
	up(&(device->sem));

	return nbytes;
}

static loff_t mycdrv_lseek(struct file *file, loff_t offset, int orig) {
	loff_t finalpos; 

	struct chardevice* device = file->private_data;

	if(down_interruptible(&(device->sem)) != 0){
		printk(KERN_ALERT "mycdrv: could not lock device during lseek");
		return -1;
	}

	switch (orig) {
	case SEEK_SET:
		finalpos = offset;
		break;
	case SEEK_CUR:
		finalpos = file->f_pos + offset;
		break;
	case SEEK_END:
		finalpos = device->size + offset;
		break;
	default:
		up(&(device->sem));
		return -EINVAL;
	}
	pr_info("size: %d, finalpos: %d",(int)device->size,(int)finalpos);

	if((int)finalpos >(int)device->size){
		size_t new_room = (size_t) finalpos - device->size;
		char* new_location = kmalloc(device->size + new_room, GFP_KERNEL); 
		memcpy(new_location, device->ramdisk, device->size); 
		kfree(device->ramdisk); 
		device->ramdisk = new_location; 
		memset((device->ramdisk) + device->size, 0, new_room); 
		device->size = device->size + new_room;
		pr_info("Memory extended. New memory is %d bytes (Old memory was %d). ",(int)device->size,(int) (device->size - new_room));
	}
	else{
		finalpos = finalpos >= 0 ? finalpos : 0;
	}
	file->f_pos = finalpos;
	pr_info("Seeking to pos=%ld\n", (long)finalpos);
	
	up(&(device->sem));

	return finalpos;
}

static long mycdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg){	
	struct chardevice* device = file->private_data;
	int err = 0;

	if(down_interruptible(&(device->sem)) != 0){
		printk(KERN_ALERT "mycdrv: could not lock device during ioctl");
		return -1;
	}

	if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC ){ 
		up(&(device->sem));
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err){
		up(&(device->sem));
		return -EFAULT;
	}

	switch (cmd) {
  	case ASP_CLEAR_BUFF:
    	pr_info("caught the clearing buffer command!");
			memset(device->ramdisk, 0, device->size); 
			file->f_pos = 0; 
      break;
    default:
      pr_info("unknown ioctl, %d", cmd);
			up(&(device->sem));
      return -EINVAL;	
	}	

	up(&(device->sem));

  return 0;
}

static const struct file_operations mycdrv_fops = {
	.owner = THIS_MODULE,
	.read = mycdrv_read,
	.write = mycdrv_write,
	.open = mycdrv_open,
	.release = mycdrv_release,
	.llseek = mycdrv_lseek,
	.unlocked_ioctl = mycdrv_ioctl
};

static int __init my_init(void) {
	int i;
	dev_t first_first,first_local;

	devices = kmalloc(NUM_DEVICES * sizeof(struct chardevice), GFP_KERNEL);
	foo_class = class_create(THIS_MODULE, "my_class"); 

	if (alloc_chrdev_region(&first_first, 0, NUM_DEVICES, MYDEV_NAME) < 0) { 
			pr_err("failed to register character device region\n");
			return -1;
	}

	for(i = 0; i < NUM_DEVICES; ++i){

		first_local = MKDEV(MAJOR(first_first), i);
		
		cdev_init(&devices[i].my_cdev, &mycdrv_fops); 
		devices[i].ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
		memset(devices[i].ramdisk, 0, ramdisk_size ); 
		devices[i].size = ramdisk_size; 
		devices[i].my_cdev.owner = THIS_MODULE;
		devices[i].my_cdev.ops = &mycdrv_fops;
		devices[i].device_num = i;
		
		if (cdev_add(&devices[i].my_cdev, first_local, count) < 0) { 
			pr_err("cdev_add() failed\n");
			cdev_del(&devices[i].my_cdev); 
			unregister_chrdev_region(first_local, count);
			kfree(devices[i].ramdisk);
			return -1;
		}

		device_create(foo_class, NULL, first_local, NULL, "%s%d", MYDEV_NAME, i);

		pr_info("\nSucceeded in registering character device %s%d", MYDEV_NAME, i);
		pr_info("Major number: %d, Minor number: %d for this device", MAJOR(devices[i].my_cdev.dev ), MINOR(devices[i].my_cdev.dev ));
		sema_init(&devices[i].sem,1);
	}
	return 0;
}

static void __exit my_exit(void) { 	
	int i;
	dev_t first = devices[0].my_cdev.dev;
	for(i = 0; i < NUM_DEVICES; i++){ 
		device_destroy(foo_class, devices[i].my_cdev.dev);
	
		if (&devices[i].my_cdev)
			cdev_del(&devices[i].my_cdev); 

		kfree(devices[i].ramdisk); 

		pr_info("device %s%d unregistered", MYDEV_NAME, i);

	}
	unregister_chrdev_region(first, NUM_DEVICES);
	pr_info("All devices unregistered");

	class_destroy(foo_class); 
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("user");
