#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <xen/interface/xen.h>
#include <xen/xen.h>
#include <asm/xen/hypercall.h>
#include <xen/events.h>
#include <linux/init.h>
#include <xen/page.h>
#include <linux/spinlock.h>
#include "xen-dom.h"
#include "pdma-ioctl.h"
MODULE_LICENSE("GPL");

#define DEVICE_PATH "/dev/pdma"

static int xen_chrif_reqs = 64;
module_param_named(reqs, xen_chrif_reqs, int, 0);
MODULE_PARM_DESC(reqs, "Number of chrback requests to allocate");
mm_segment_t old_fs;
static DEFINE_MUTEX(pdma_lock);

struct pending_req {
	struct xen_chrif	*chrif;
	u64			id;
	int			nr_pages;
	atomic_t		pendcnt;
	unsigned short		operation;
	int			status;
	struct list_head	free_list;
};

struct xen_chrbk {
	struct pending_req	*pending_reqs;
	/* List of all 'pending_req' available */
	struct list_head	pending_free;
	/* And its spinlock. */
	spinlock_t		pending_free_lock;
	wait_queue_head_t	pending_free_wq;
	/* The list of all pages that are available. */
	struct page		**pending_pages;
	/* And the grant handles that are available. */
	grant_handle_t		*pending_grant_handles;
	struct   file *chrif_filp;
	int   open_cnt;
	int   pdma_dma_cnt;
};

static struct xen_chrbk *chrbk;

/*
 * Retrieve from the 'pending_reqs' a free pending_req structure to be used.
 */
static struct pending_req *alloc_req(void)
{
	struct pending_req *req = NULL;
	unsigned long flags;

	spin_lock_irqsave(&chrbk->pending_free_lock, flags);
	if (!list_empty(&chrbk->pending_free)) {
		req = list_entry(chrbk->pending_free.next, struct pending_req,
				 free_list);
		list_del(&req->free_list);
	}
	spin_unlock_irqrestore(&chrbk->pending_free_lock, flags);
	return req;
}

/*
 * Return the 'pending_req' structure back to the freepool. 
 * wake up the thread if it was waiting for a free page.
 */
static void free_req(struct pending_req *req)
{
	unsigned long flags;
	int was_empty;

	spin_lock_irqsave(&chrbk->pending_free_lock, flags);
	was_empty = list_empty(&chrbk->pending_free);
	list_add(&req->free_list, &chrbk->pending_free);
	spin_unlock_irqrestore(&chrbk->pending_free_lock, flags);
	if (was_empty)
		wake_up(&chrbk->pending_free_wq);
}

/*
 * Notification from the guest OS.
 */
static void chrif_notify_request(struct xen_chrif  *chrif)
{
	chrif->waiting_reqs =1;
	wake_up(&chrif->wq);
}

irqreturn_t chrif_int(int irq, void *dev_id)
{
	chrif_notify_request(dev_id);
    printk(KERN_INFO "\n------------------------------start response-------------------------------------");
    printk(KERN_DEBUG "\nxen: Dom0: chrif_int called with dev_id=%x ", (unsigned int)dev_id);

	return IRQ_HANDLED;
}


static void do_write_resp(struct xen_chrif *xen_chrif, struct chrif_request *req, struct chrif_response *resp)
{
/**    resp->rdwr.len = req->rdwr.len;

    memcpy(block_buf+wr_time*4096, op_page->addr, 4096);
	old_fs = get_fs();
	set_fs(get_ds());
	//write data from page to device  
	err = chrbk->chrif_filp->f_op->write(chrbk->chrif_filp, block_buf, resp.rdwr.len, &chrbk->chrif_filp->f_pos);	
	set_fs(old_fs);
    if(err < 0)
		printk(KERN_DEBUG "\nxen: Dom0: write %u bytes error", resp->rdwr.len);
     memset(block_buf, 0, resp->rdwr.len);

    printk(KERN_DEBUG "\nxen: dom0: response write");

	memset(op_page->addr, 0, 4096);
	old_fs = get_fs();
	set_fs(get_ds());
	//read data from device to page 
	err = chrbk->chrif_filp->f_op->read(chrbk->chrif_filp, block_buf, resp.rdwr.len, &chrbk->chrif_filp->f_pos);	
	set_fs(old_fs);
    if(err < 0)
		printk(KERN_DEBUG "\nxen: Dom0: read %u bytes error", resp->rdwr.len);
    memcpy(op_page->addr, block_buf+rd_time*4096, 4096);
    memset(block_buf, 0, resp->rdwr.len);
	**/
}

static int do_ioctl_resp(struct chrif_request *req, struct chrif_response *resp)
{
	int err = 0;
    resp->ioc_parm.cmd = req->ioc_parm.cmd;

	switch(resp->ioc_parm.cmd){
		case PDMA_IOC_START_DMA:{
			chrbk->pdma_dma_cnt++;
			//only pdma_start when first to start dma
			if (chrbk->pdma_dma_cnt == 1){
				old_fs = get_fs();
				set_fs(get_ds());
				err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, PDMA_IOC_START_DMA, NULL);	
				set_fs(old_fs);
			}
			if(err){
				printk(KERN_DEBUG "\nxen: Dom0: start-dma ioctl failed");
			}else  printk(KERN_DEBUG "\nxen: Dom0: start-dma ioctl success");
			break;
		}

		case PDMA_IOC_STOP_DMA:{
			chrbk->pdma_dma_cnt--;
			//only pdma_stop when last to stop dma
			if (chrbk->pdma_dma_cnt == 0){
				old_fs = get_fs();
				set_fs(get_ds());
				err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, PDMA_IOC_STOP_DMA, NULL);	
				set_fs(old_fs);
			}
			if (err){
				printk(KERN_DEBUG "\nxen: Dom0: stop-dma ioctl failed");
			}else  printk(KERN_DEBUG "\nxen: Dom0: stop-dma ioctl success");
			break;
		}

		case PDMA_IOC_INFO:{
			struct pdma_info pdma_info;
			old_fs = get_fs();
			set_fs(get_ds());
			err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, PDMA_IOC_INFO, (unsigned long)&pdma_info);	
			set_fs(old_fs);
			if(err){
				printk(KERN_DEBUG "\nxen: Dom0: info ioctl failed");
			}else  printk(KERN_DEBUG "\nxen: Dom0: info ioctl success");
			resp->ioc_parm.info = pdma_info;
			printk(KERN_DEBUG "\nxen: Dom0: rd_pool_sz = 0x%lx", resp->ioc_parm.info.rd_pool_sz);
			
            break;
		}

		case PDMA_IOC_STAT:{
			struct pdma_stat pdma_stat;
			old_fs = get_fs();
			set_fs(get_ds());
			err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, PDMA_IOC_STAT, (unsigned long)&pdma_stat);	
			set_fs(old_fs);
			if(err){
				printk(KERN_DEBUG "\nxen: Dom0: stat ioctl failed");
			}else  printk(KERN_DEBUG "\nxen: Dom0: stat ioctl success");
			resp->ioc_parm.stat = pdma_stat;
			break;
		}

		case PDMA_IOC_RW_REG:{
			struct pdma_rw_reg ctrl = req->ioc_parm.ctrl;
			old_fs = get_fs();
			set_fs(get_ds());
			err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
			set_fs(old_fs);
			if(err){
				printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
			}else  printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl success");
			resp->ioc_parm.ctrl = ctrl;
			break;
		}

		default:
			printk(KERN_DEBUG "\nxen: Dom0: unknow the operation");
            break;
	}

	printk(KERN_INFO "\nxen: Dom0: response ioctl");

	return err;
}

static void make_response(struct xen_chrif *xen_chrif, struct chrif_request *req, struct chrif_response *response)
{
	struct xen_chrif  *chrif = xen_chrif;
	struct chrif_response *resp = response;
	unsigned long flags;
	int notify;

	resp->id = req->id;
    resp->operation = req->operation;
    resp->status = req->status + 1;
	
    spin_lock_irqsave(&chrif->chr_ring_lock, flags);
    
	memcpy(RING_GET_RESPONSE(&chrif->chr_ring, chrif->chr_ring.rsp_prod_pvt), resp, sizeof(*resp));
    chrif->chr_ring.rsp_prod_pvt++;
    RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&chrif->chr_ring, notify);
	spin_unlock_irqrestore(&chrif->chr_ring_lock, flags);
       
    printk(KERN_DEBUG "\nxen:Dom0: make response id = %d, op=%s, status=%d",
		resp->id, op_name(resp->operation), resp->status);
	
    if (notify) {
		printk(KERN_DEBUG "\nxen:dom0:send notify to domu");
        notify_remote_via_irq(chrif->irq);
     }
}

static int __do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	struct pending_req  *pending_req;
	unsigned long  flags;
    struct chrif_request  *req;
	chrif_response_t  *resp;
	RING_IDX  rc, rp;    
	int more_to_do = 0;
    resp = kmalloc(sizeof(*resp), GFP_KERNEL);
 
	rc = chrif->chr_ring.req_cons;
    rp = chrif->chr_ring.sring->req_prod;
	rmb();
    printk(KERN_DEBUG "\nxen: Dom0: rc = %d rp = %d", rc, rp);

	while (rc != rp) {
       if (RING_REQUEST_CONS_OVERFLOW(&chrif->chr_ring, rc))
           break;
       
	   if(kthread_should_stop()){
		   more_to_do = 1;
		   break;
	   }

	   pending_req = alloc_req();
	   if (NULL == pending_req) {
			more_to_do = 1;
			break;
		}

	   //memcpy(req, RING_GET_REQUEST(&chrif->chr_ring, rc), sizeof(*req));
       req = RING_GET_REQUEST(&chrif->chr_ring, rc);
       chrif->chr_ring.req_cons = ++rc;
      
       barrier();
       printk(KERN_DEBUG "\nxen:Dom0:Recvd at IDX-%d: id = %d, op=%s, status=%d", 
		   rc, req->id, op_name(req->operation), req->status);
 
	   mutex_lock(&pdma_lock);
	   switch(req->operation) {
			case CHRIF_OP_OPEN:
				chrbk->open_cnt++;
				//only open when the first one to open
                if (chrbk->open_cnt == 1)
					 chrbk->chrif_filp = filp_open(DEVICE_PATH, O_RDWR, 0);
				printk(KERN_DEBUG "\nxen: dom0: response open");
                break;
			
			case CHRIF_OP_WRITE:
				do_write_resp(chrif, req, resp);
				break;
			
			case CHRIF_OP_IOCTL:
				if (do_ioctl_resp(req, resp))
					req->status-=1;
				break;

			case CHRIF_OP_CLOSE:
				chrbk->open_cnt--;
				//only close when the last to close
				if (chrbk->open_cnt == 0)
					filp_close(chrbk->chrif_filp, NULL);
			
				printk(KERN_INFO "\nxen: Dom0: response close");
				break;
			
            default:
                printk(KERN_DEBUG "\nxen: Dom0: unknow the operation");
				req->status-=1;
                break;
       }

	   free_req(pending_req);
       make_response(chrif, req, resp);
	   mutex_unlock(&pdma_lock);
	}

    return more_to_do;
}

static int do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	int more_to_do;

	do {
		more_to_do = __do_char_io_op(chrif);
		if (more_to_do)
			break;

		RING_FINAL_CHECK_FOR_REQUESTS(&chrif->chr_ring, more_to_do);
	} while (more_to_do);

	return more_to_do;
}

/*
*the entry function of thread
*/
int xen_chrif_schedule(void *arg)
{
	struct xen_chrif *chrif = arg;
	while(!kthread_should_stop()){
		wait_event_interruptible(chrif->wq, 
					            chrif->waiting_reqs||
                                kthread_should_stop());
		wait_event_interruptible(chrbk->pending_free_wq,
								!list_empty(&chrbk->pending_free)||
								kthread_should_stop());
		chrif->waiting_reqs = 0;
		smp_mb();

		if(do_char_io_op(chrif))
			chrif->waiting_reqs = 1;
	}

	chrif->xenchrd = NULL;

	return 0;
}

static int dom_init(void) 
{
    int i;
	int err = 0;
    
    chrbk = kzalloc(sizeof(struct xen_chrbk), GFP_KERNEL);
    if(!chrbk){
        printk(KERN_DEBUG "\nxen: dom0: out of memoty when alloc chrbk");
    }

	chrbk->pending_reqs = kzalloc(sizeof(chrbk->pending_reqs[0]) *xen_chrif_reqs, GFP_KERNEL);

	INIT_LIST_HEAD(&chrbk->pending_free);
	spin_lock_init(&chrbk->pending_free_lock);
	init_waitqueue_head(&chrbk->pending_free_wq);

	chrbk->open_cnt = 0;
	chrbk->pdma_dma_cnt = 0;

	for (i = 0; i < xen_chrif_reqs; i++)
		list_add_tail(&chrbk->pending_reqs[i].free_list, &chrbk->pending_free);
    
	err = xen_chrif_xenbus_init();
    if(err)
		goto failed_init;

	return 0;

failed_init:
	kfree(chrbk->pending_reqs);
	kfree(chrbk);
	chrbk = NULL;
	return err;
}

static void dom_exit(void)
{
    xen_chrif_xenbus_exit( );
}

module_init(dom_init);
module_exit(dom_exit);
 
