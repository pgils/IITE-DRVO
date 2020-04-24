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

#define DEVICE_NAME "drvoscd"   /* DRiVerOntwikkeling Sensor Character Driver */
#define CLASS_NAME  "drvo"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("0.1");

static int              majorNumber;
static char             message[256] = {0};
static short            size_of_message;
static struct class*    drvoscdClass = NULL;
static struct device*   drvoscdDevice = NULL;

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
   printk(KERN_INFO "DRVOSCD: Initializing the DRVOSCD LKM\n");

   // dynamically allocate a major number for the device
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "DRVOSCD failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "DRVOSCD: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   drvoscdClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(drvoscdClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(drvoscdClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "DRVOSCD: device class registered correctly\n");
 
   // Register the device driver
   drvoscdDevice = device_create(drvoscdClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(drvoscdDevice)){               // Clean up if there is an error
      class_destroy(drvoscdClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(drvoscdDevice);
   }
   printk(KERN_INFO "DRVOSCD: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

static void __exit drvoscd_exit(void){
   device_destroy(drvoscdClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(drvoscdClass);                          // unregister the device class
   class_destroy(drvoscdClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "DRVOSCD: de-initialised LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   return 0;
}

// https://unix.stackexchange.com/a/371758
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    int error = 0;

    // `cat` will keep reading untill it receives a 0 (EOF)
    // use offset to signal the message has already been read.
    if (*offset != 0) {
        return error;
    }
    // copy message from kernel- to user-space
    // copy_to_user has the format ( * to, *from, size) and returns 0 on success
    error = copy_to_user(buffer, message, size_of_message);
 
    if (error == 0){
        // set offset so next read will return 0 (EOF)
        *offset = size_of_message;
        return size_of_message;
    }
    else {
        printk(KERN_INFO "DRVOSCD: Failed to send %d characters to the user\n", error);
        return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    // copy message from user- to kernel-space
   if(copy_from_user(message, buffer, len)) {
       return -EFAULT;
   }
   size_of_message = strlen(message);                 // store the length of the stored message
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "DRVOSCD: Device successfully closed\n");
   return 0;
}

module_init(drvoscd_init);
module_exit(drvoscd_exit);