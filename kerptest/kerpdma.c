#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/proc_ns.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <asm/sync_bitops.h>
#include <asm/uaccess.h>
#include <asm/device.h>
#include <linux/slab.h>
#include <asm-generic/delay.h>

#include "pdma-ioctl.h"
MODULE_LICENSE("GPL");


#define DEV_NAME "kerp" 
#define DEVICE_PATH "/dev/pdma"

mm_segment_t old_fs;
static struct class *domt_class;
dev_t dev_no=0;

struct  kerp_info{
	struct   file *kerp_filp;
	struct cdev cdev;
	char *data_buf;
	struct pdma_info pdma_info;
};



static  int kerp_open(struct inode *inode, struct file *filp)
{
	struct kerp_info *info;
	
	printk(KERN_DEBUG "\nxen: DomU: start open");
    info = container_of(inode->i_cdev, struct kerp_info, cdev);
	info->kerp_filp = filp_open(DEVICE_PATH, O_RDWR, 0);

    filp->private_data = info;
    printk(KERN_DEBUG "\nxen: DomU: contain info finished");
    //filled with request

    return 0;
    
}

static ssize_t kerp_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	struct kerp_info *info = filp->private_data;
	
	if(__copy_to_user(buf, info->data_buf, len))
        return -EFAULT;
		
	return len;	
}

static ssize_t kerp_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
    int err, i;
	struct pdma_rw_reg ctrl;
	int count = 0;
    struct kerp_info *info = filp->private_data;
	char * buffer;
	int size = info->pdma_info.wt_block_sz;
	
	printk(KERN_DEBUG "\nxen: DomU: file_size %d len", len);
	if(len%size)
		count = len/size + 1;
	else
		count = len/size;
	printk(KERN_DEBUG "\nxen: DomU: file_write %d times", count);
	
	buffer = (char *)kmalloc(count*size, GFP_KERNEL);	
	if (copy_from_user(buffer, buf, len))
        return -EFAULT;
		
    //udelay(70);
	for(i = 0; i < count; i++){
		old_fs = get_fs();
		set_fs(get_ds()); 
		err = info->kerp_filp->f_op->write(info->kerp_filp, buffer+i*size, 
								size, &info->kerp_filp->f_pos);	
		set_fs(old_fs);
		if(err < 0){
			printk("\nkerp: write %u bytes error", size);
			return err;
		}
	}
	
	udelay(5);
	//read reg
	ctrl.type = 0; //read
	ctrl.addr  = 0;
	old_fs = get_fs();
	set_fs(get_ds());
	err = info->kerp_filp->f_op->unlocked_ioctl(info->kerp_filp, PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
	set_fs(old_fs);
	printk(KERN_DEBUG "\nxen: Dom0: read form reg: %d", ctrl.val);
	if(err){
		printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
	}
	
	//read data
	old_fs = get_fs();
	set_fs(get_ds());
	err = info->kerp_filp->f_op->read(info->kerp_filp, info->data_buf, 
									size, &info->kerp_filp->f_pos);	
	printk(KERN_DEBUG "\nkerp: info->pdma_info.wt_block_sz: %d", info->pdma_info.wt_block_sz);
	set_fs(old_fs);
	if(err < 0){
		printk(KERN_DEBUG "\nkerp: read %u bytes error", len);
		return err;
	}
	printk(KERN_DEBUG "\nkerp: read data from pdma");
	
	
	return len;
}

long kerp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    struct kerp_info *info = filp->private_data;
	
    switch(cmd) {
		/*pdma dma*/
		case PDMA_IOC_START_DMA : 
		case PDMA_IOC_STOP_DMA:
			break;
			
		/*pdma info*/
		case PDMA_IOC_INFO:{
			old_fs = get_fs();
			set_fs(get_ds());	
			ret =  info->kerp_filp->f_op->unlocked_ioctl(info->kerp_filp, PDMA_IOC_INFO, (unsigned long)&info->pdma_info);	
			set_fs(old_fs);
			
			if (copy_to_user((void *)arg, &info->pdma_info, sizeof(struct pdma_info))) {
                ret = -EFAULT;
            }
			break;
		}
		
        /* read/write register */
        case PDMA_IOC_RW_REG: {  
			struct pdma_rw_reg ctrl;
			if (copy_from_user(&ctrl, (void *)arg, sizeof(struct pdma_rw_reg))) {
				ret = -EFAULT;
				break;
			}
			if(ctrl.type == 0){ //read
				old_fs = get_fs();
				set_fs(get_ds());
				ret = info->kerp_filp->f_op->unlocked_ioctl(info->kerp_filp, PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
				set_fs(old_fs);
				if (copy_to_user((void *)arg, &ctrl, sizeof(struct pdma_rw_reg))) {
					ret = -EFAULT;
				}
			}else{
				old_fs = get_fs();
				set_fs(get_ds());
				ret = info->kerp_filp->f_op->unlocked_ioctl(info->kerp_filp, PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
				set_fs(old_fs);
			}
			
			break;
        }

        /* pdma stat */
        case PDMA_IOC_STAT: 
			break;

        default:  /* redundant, as cmd was checked against MAXNR */
			break;
    }
     
    printk(KERN_INFO "\nxen: DOmU: ioctl op finished");

    return ret;
}


static int kerp_release(struct inode *inode, struct file *filp)
{
	struct kerp_info *info = filp->private_data;
	
	filp_close(info->kerp_filp, NULL);
	printk("close->pdma\n");
	return 0;
}


struct file_operations kerp_fops=
{
    .open = kerp_open,
    .read = kerp_read,
    .write = kerp_write,
    .release = kerp_release,
    .owner = THIS_MODULE,
    // #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl =  kerp_ioctl,
    // #else
    // .ioctl          =  kerp_ioctl,
    // #endif
};

static int create_chrdev(struct  kerp_info* info)
{
    int ret,err;

    //allocate device number dynamically
    ret=alloc_chrdev_region(&dev_no, 0, 1, DEV_NAME);
    if(ret)
    {
        printk(KERN_DEBUG "\nkerp: pdma register failure");
        unregister_chrdev_region(dev_no, 1);
        return ret;
    }
    else
    {
        printk(KERN_DEBUG "\nkerp: pdma register success");
    }
    cdev_init(&info->cdev, &kerp_fops);
    info->cdev.owner = THIS_MODULE;
    info->cdev.ops = &kerp_fops;

    err = cdev_add(&info->cdev, dev_no, 1);
    if(err)
    {
        printk(KERN_DEBUG "\nkerp: error %d adding pdma", err);
        unregister_chrdev_region(dev_no, 1);
        return err;
    }
    domt_class=class_create(THIS_MODULE, DEV_NAME);
    if(IS_ERR(domt_class))
    {
        printk(KERN_DEBUG "\nkerp: ERR:cannot create a domt_class");  
        unregister_chrdev_region(dev_no, 1);
        return -1;
    }
    device_create(domt_class, NULL, dev_no, 0, DEV_NAME);
    return ret;
}

static int __init kerp_init(void)
{
    int ret;
	struct  kerp_info* info;
    printk(KERN_DEBUG "\nkerp: START init......................\n");
    
	info = kmalloc(sizeof(*info), GFP_KERNEL);
    info->data_buf = (char *)kmalloc(16384, GFP_KERNEL);
	memset(info->data_buf, 0, 16384);
	create_chrdev(info);
	
   	printk(KERN_ALERT"\nkerp: driver register success");

   	return 0;
}

static void __exit kerp_exit(void)
{
    device_destroy(domt_class, dev_no);
    class_destroy(domt_class);
    unregister_chrdev_region(dev_no,1);

    printk(KERN_DEBUG "\nkerp: END exit......................\n");
 }
 

module_init(kerp_init);
module_exit(kerp_exit);
