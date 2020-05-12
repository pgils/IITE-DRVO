/*
 * A simple character device driver
 * based on http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEVICE_NAME "drvoscd"   /* DRiVerOntwikkeling Sensor Character Driver */
#define CLASS_NAME  "drvo"

#define sysfs_file_len  64

MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("0.1");

static int              majorNumber;
static struct class*    drvoscdClass = NULL;
static struct device*   drvoscdDevice = NULL;

static char**           sysfs_path = NULL;
char*                   sysfs_file_temp = "/sys/bus/iio/devices/iio:device0/in_temp_input";
char*                   sysfs_file_pres = "/sys/bus/iio/devices/iio:device0/in_pressure_input";


static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init drvoscd_init(void){
   printk(KERN_INFO "%s: Initializing the DRVOSCD LKM\n", DEVICE_NAME);

   // dynamically allocate a major number for the device
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "%s failed to register a major number\n", DEVICE_NAME);
      return majorNumber;
   }
   printk(KERN_INFO "%s: registered correctly with major number %d\n", DEVICE_NAME, majorNumber);
 
   // Register the device class
   drvoscdClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(drvoscdClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(drvoscdClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "%s: device class registered correctly\n", DEVICE_NAME);
 
   // Register the device driver
   drvoscdDevice = device_create(drvoscdClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(drvoscdDevice)){               // Clean up if there is an error
      class_destroy(drvoscdClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(drvoscdDevice);
   }
   printk(KERN_INFO "%s: device class created correctly\n", DEVICE_NAME); // Made it! device was initialized
   return 0;
}

static void __exit drvoscd_exit(void){
   device_destroy(drvoscdClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(drvoscdClass);                          // unregister the device class
   class_destroy(drvoscdClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "%s: de-initialised LKM!\n", DEVICE_NAME);
}

static int dev_open(struct inode *inodep, struct file *filep){
   return 0;
}

// https://unix.stackexchange.com/a/371758
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    struct file     *data_fp;
    mm_segment_t    data_fs;

    // `cat` will keep reading untill it receives a 0 (EOF)
    // use offset to signal the message has already been read.
    if (*offset != 0) {
        return 0;
    }

    // sysfs path has to be set before reading
    if (sysfs_path == NULL) {
        printk(KERN_INFO "%s: sysfs target is not set", DEVICE_NAME);
        return -EFAULT;
    }

    data_fp = filp_open(*sysfs_path, O_RDONLY, 0);
    if (data_fp == NULL) {
        printk(KERN_ALERT "%s: filp_open failed", DEVICE_NAME);
        return -EFAULT;
    }

    // Get current segment descriptor
    data_fs = get_fs();
    // Set segment descriptor associated to kernel space
    set_fs(KERNEL_DS);
    // Read the file
    data_fp->f_op->read(data_fp, buffer, sysfs_file_len, &data_fp->f_pos);
    // Restore segment descriptor
    set_fs(data_fs);

    // set offset so next read will return 0 (EOF)
    // *offset = size_of_message;
    *offset = sysfs_file_len;
    // return size_of_message;
    return sysfs_file_len;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    char    message[256] = {0};

    // copy message from user- to kernel-space
   if(copy_from_user(message, buffer, len)) {
       return -EFAULT;
   }
   // set the sysfs file to be read based on the provided input
   // only 4 characters are compared. so 'temperature' or 'pressure' would be accepted as well.
   if (strncmp(message, "temp", 4) == 0) {
       sysfs_path = &sysfs_file_temp;
   } else if (strncmp(message, "pres", 4) == 0) {
       sysfs_path = &sysfs_file_pres;
   } else {
       printk(KERN_INFO "%s: Unknown argument provided: %s\n", DEVICE_NAME, message);
   }

   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "%s: Device successfully closed\n", DEVICE_NAME);
   return 0;
}

module_init(drvoscd_init);
module_exit(drvoscd_exit);