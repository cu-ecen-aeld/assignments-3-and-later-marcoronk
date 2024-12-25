/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Marco Ronchini"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    struct aesd_dev *dev;

	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = dev; 
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}
int checkCR(char *buffer,int size) {

  int ret = -1;
  if (buffer) {   
    for (int c=0; c < size; c++) {
        if (buffer[c]== '\n') {
           return c;
        }                 
    }
  }
  PDEBUG("Check cr for buffer ret %lld",ret);
  return ret;
}



ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    ssize_t btocopy = 0;
    struct aesd_dev *dev = filp->private_data; 
    struct aesd_buffer_entry *entry = NULL;
    size_t entry_offset_byte_rtn;
    PDEBUG("READ  %zu bytes with offset %lld",count,*f_pos);  
    if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->aesd_buffer,*f_pos,&entry_offset_byte_rtn);
    if(entry == NULL) {
       PDEBUG("No entry found ");  
       mutex_unlock(&dev->lock);
       return retval;
    }
    btocopy = entry->size - entry_offset_byte_rtn;
    if (count > btocopy)
        retval = btocopy;
    else 
        retval = count;    
    if (copy_to_user(buf, entry->buffptr + entry_offset_byte_rtn, retval)) {
        PDEBUG("Error copy_to_user ");  
        mutex_unlock(&dev->lock);
        retval = -EFAULT;
        return retval;
    }    
    *f_pos += retval;
    mutex_unlock(&dev->lock);
    return retval;
}


ssize_t sendEntryToCircularBuffer(struct aesd_dev *dev){

    int posCR=-1;
    char *lastEntry=NULL;
    posCR = checkCR(dev->aesd_entry.buffptr,dev->aesd_entry.size);
    if ((posCR >= 0) && (posCR == dev->aesd_entry.size-1)) {   
       aesd_circular_buffer_add_entry(&dev->aesd_buffer,&dev->aesd_entry);
       dev->aesd_entry.buffptr = NULL;
       dev->aesd_entry.size = 0;
    } 

    return 0;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_buffer_entry *entry;
    size_t entry_offset_byte_rtn;
    int size=0;
    char* locBuffer = NULL;
    struct aesd_dev *dev = filp->private_data; 

    PDEBUG("WRITE %zu bytes with offset %lld",count);
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    PDEBUG("scrivo %s",buf);        
   
   
    if (dev->aesd_entry.buffptr) {
        size = count+dev->aesd_entry.size;      
        locBuffer = kmalloc(size,GFP_KERNEL);
        if (!locBuffer) {
           PDEBUG("errore nell'allocazione di memoria %lld",size);
           retval = -EINVAL;
           mutex_unlock(&dev->lock);
           return retval;
        }
        memset(locBuffer,size,0);
        memcpy(locBuffer,dev->aesd_entry.buffptr,dev->aesd_entry.size);
        if (copy_from_user(locBuffer+dev->aesd_entry.size, buf, count)) {
         PDEBUG("Errore CopyUser 1");
         retval = -EINVAL;
         mutex_unlock(&dev->lock);
         return retval;
        }
        kfree(dev->aesd_entry.buffptr);
        dev->aesd_entry.buffptr = locBuffer;
        dev->aesd_entry.size = size;
        sendEntryToCircularBuffer(dev);
    }
    else {
        dev->aesd_entry.buffptr = kmalloc(count,GFP_KERNEL);
        if (copy_from_user(dev->aesd_entry.buffptr, buf, count)) {       
         retval = -EINVAL;
         mutex_unlock(&dev->lock);
         return retval;
        }                        
        dev->aesd_entry.size = count;
        sendEntryToCircularBuffer(dev);
    }
    mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);
    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}


int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,"aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    aesd_circular_buffer_init(&aesd_device.aesd_buffer);
 
    aesd_device.aesd_entry.buffptr  = NULL;
    aesd_setup_cdev(&aesd_device);

    result = aesd_setup_cdev(&aesd_device);
    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    PDEBUG("aesd_cleanup_module");    	
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
