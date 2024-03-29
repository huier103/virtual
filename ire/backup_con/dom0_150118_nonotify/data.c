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

static int xen_chrif_reqs = 64;
module_param_named(reqs, xen_chrif_reqs, int, 0);
MODULE_PARM_DESC(reqs, "Number of chrback requests to allocate");
mm_segment_t old_fs;
static DEFINE_MUTEX(pdma_lock);

struct pending_req {
	struct xen_chrif	*chrif;
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
	/*the mark to operate pdma*/
	int   open_cnt;
	int   pdma_dma_cnt;
	int wting_domu_id;
	unsigned int writed_sz;
	/*wait for readed data transportation */
	struct completion com_read;
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
 * map fronted wr_pages
 */
static int xen_chrbk_map(struct chrif_request *request, struct xen_chrif *xen_chrif)
{
	int i, count;
    unsigned long *map_page;
    struct vm_struct *vm_area;
	struct chrif_request *req = request;
	struct xen_chrif *chrif = xen_chrif;
	struct gnttab_map_grant_ref map_ops[MAX_GREF];
	chrif->nr_pages = req->nr_pages;
    count = req->nr_pages;

	for(i = 0; i < count; i++){
		map_page = &chrif->map_pages[i];
        vm_area = alloc_vm_area(PAGE_SIZE, NULL);
		*map_page = (unsigned long)vm_area->addr;
        gnttab_set_map_op(&map_ops[i], (unsigned long)vm_area->addr, 
			GNTMAP_host_map, req->op_gref[i], chrif->domid);
   

		if(HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &map_ops[i], 1)){
       		printk(KERN_DEBUG "\nxen: dom0: map pages failed");
			return -EFAULT;
		}

		if (map_ops[i].status) {
			struct gnttab_unmap_grant_ref unmap_op;
			gnttab_set_unmap_op(&unmap_op, (unsigned long)vm_area->addr, 
				GNTMAP_host_map, map_ops[i].handle);
			HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_op, 1);
			printk(KERN_DEBUG "\nxen: dom0: map_op.status fail");
			return map_ops[i].status;
		}

       	printk(KERN_DEBUG "\nxen: dom0: vm_area->addr[%d]: 0x%x ", 
        i, vm_area->addr);
       	printk(KERN_DEBUG "\nxen: dom0: chrif->map_pages[%d]: 0x%x ",
        i, chrif->map_pages[i]);
/*		printk(KERN_DEBUG "\nxen: dom0: vm_area->data[%d]: 0x%d ", 
        i, *((int *)vm_area->addr));
		printk(KERN_DEBUG "\nxen: dom0: chrif->data[%d]: 0x%d ", 
        i, *((int *)chrif->map_pages[i]));
*/
		chrif->pages_handle[i] = map_ops[i].handle;
       	printk(KERN_DEBUG "\nxen: dom0: map %d page finished, handle: %d", 
            i, chrif->pages_handle[i]);
	}

	return 0;
}

/*
 * unmap fronted wt_pages
 */
void xen_chrbk_unmap(struct xen_chrif *chrif)
{
    int i;
    unsigned long map_page;
	struct gnttab_unmap_grant_ref unmap_ops[MAX_GREF];
	int count = chrif->nr_pages;

	for(i = 0; i < count; i++){
        map_page = chrif->map_pages[i];
		gnttab_set_unmap_op(&unmap_ops[i], (unsigned long)map_page, 
				GNTMAP_host_map, chrif->pages_handle[i]);
		HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops[i], 1);
	}

	//free_vm_area(chrif->map_pages);
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

static int do_read_resp(struct xen_chrif *xen_chrif, struct chrif_request *req, struct chrif_response *resp)
{
	int err, i;
	struct xen_chrif *chrif = xen_chrif;
	unsigned int readed_sz;
	unsigned long map_page;
	int count = req->nr_pages;

    /* send all data to frontend, every time send a blcok */
 	for(readed_sz = 0; readed_sz < req->length; ){
		old_fs = get_fs();
		set_fs(get_ds());
		err = chrbk->chrif_filp->f_op->read(chrbk->chrif_filp, chrif->block_buf, 
									chrif->pdma_info.wt_block_sz, &chrbk->chrif_filp->f_pos);	
		set_fs(old_fs);
		if(err < 0){
			printk(KERN_DEBUG "\nxen: Dom0: read %u bytes error", req->length);
			return err;
		}
		printk(KERN_DEBUG "\nxen: Dom0: read data from pdma");

		/* write data from buffer to pages */
		for(i = 0; i < count; i++){
			map_page = chrif->map_pages[i];
			memcpy(map_page, chrif->block_buf + i*4096, 4096);
		}
		memset(chrif->block_buf, 0, chrif->pdma_info.wt_block_sz);
		printk(KERN_DEBUG "\nxen: Dom0: put read data into page");
			
		req->operation = CHRIF_OP_READ;
		resp->length = req->length;
		make_response(chrif, req, resp);
		printk(KERN_DEBUG "\nxen: Dom0: response read");
		
		wait_for_completion(&chrbk->com_read);
		printk(KERN_DEBUG "\nxen: Dom0: complete com_read");

        readed_sz += chrif->pdma_info.wt_block_sz;
	}

	chrbk->wting_domu_id = -1;
	kfree(chrif->block_buf);

	return 0;
}

static int do_write_resp(struct xen_chrif *xen_chrif, struct chrif_request *req, struct chrif_response *resp)
{
	int i, j, k, err;
	unsigned long map_page;
    struct xen_chrif *chrif = xen_chrif;
	int count = req->nr_pages;

	printk(KERN_DEBUG "\nxen: Dom0: write: nr-pages %d", count);

	/*the first time to write data, alloc buffer*/
	if(chrbk->writed_sz == 0){
		chrif->block_buf = (char *) kmalloc(chrif->pdma_info.wt_block_sz, GFP_KERNEL);
		memset(chrif->block_buf, 0, chrif->pdma_info.wt_block_sz);
	    printk(KERN_DEBUG "\nxen: Dom0: write: alloc buffer");
	}

	/*write data from pages to buffer */
	for(i = 0; i < count; i++){
        map_page = chrif->map_pages[i];
		printk(KERN_DEBUG "\nxen: dom0: the %d page addr: 0x%x ",
        i, map_page);
		memcpy(chrif->block_buf + i*4096, map_page, 4096);
	    //for test ,output data
/*		for(k = 0; k < 20; k++)
			printk(KERN_DEBUG "\nxen: dom0: chrif->data[%d]: 0x%d ", 
			i, ((int *)map_page)[k]);
*/
        memset((char *)map_page, 0, 4096);
	}
	printk(KERN_DEBUG "\nxen: Dom0: write: read all pages");
	
    /* write data from buffer to  pdma device */ 
	old_fs = get_fs();
	set_fs(get_ds()); 
	err = chrbk->chrif_filp->f_op->write(chrbk->chrif_filp, chrif->block_buf, 
							chrif->pdma_info.wt_block_sz, &chrbk->chrif_filp->f_pos);	
	set_fs(old_fs);
    if(err < 0){
		printk(KERN_DEBUG "\nxen: Dom0: write %u bytes error", chrif->pdma_info.wt_block_sz);
		return err;
	}
    memset(chrif->block_buf, 0, chrif->pdma_info.wt_block_sz);
	chrbk->writed_sz += chrif->pdma_info.wt_block_sz;
	printk(KERN_DEBUG "\nxen: Dom0: write: writed into pdma");
	 		
	/*after finishing all write, read data from pdma device, send to frontend */
    //response write
    make_response(chrif, req, resp);
	printk(KERN_DEBUG "\nxen: dom0: response write, and writed_sz: %u", chrbk->writed_sz);
  
    if(chrbk->writed_sz == req->length){        
		chrbk->writed_sz = 0;       
		printk(KERN_DEBUG "\nxen: Dom0: write finished ,call do_read_resp");
	    do_read_resp(chrif, req, resp);		
		printk(KERN_DEBUG "\nxen: Dom0: read finished ");
	}

	return 0;
}

static int do_ioctl_resp(struct xen_chrif *chrif, struct chrif_request *req, struct chrif_response *resp)
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
			resp->ioc_parm.info = chrif->pdma_info;
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
			req->status-=1;
			printk(KERN_DEBUG "\nxen: Dom0: unknow the operation");
            break;
	}

	make_response(chrif, req, resp);
	printk(KERN_INFO "\nxen: Dom0: response ioctl");

	return err;
}


static int __do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	struct pending_req  *pending_req;
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
 
wtrelock:
	   mutex_lock(&pdma_lock);
	   switch(req->operation) {
			case CHRIF_OP_OPEN:
				chrbk->open_cnt++;
				//only open when the first one to open
                if (chrbk->open_cnt == 1)
				    chrbk->chrif_filp = filp_open(DEVICE_PATH, O_RDWR, 0);

				make_response(chrif, req, resp);
				printk(KERN_DEBUG "\nxen: dom0: response open");
                break;
			
			case CHRIF_OP_WRITE:{
				if(chrbk->wting_domu_id == -1)
					chrbk->wting_domu_id = chrif->domid;
				if(chrbk->wting_domu_id != chrif->domid){
					mutex_unlock(&pdma_lock);
					goto wtrelock;
				 }

				 if(do_write_resp(chrif, req, resp))
					 printk(KERN_DEBUG "\nxen: dom0: response write fail");
				 else printk(KERN_DEBUG "\nxen: dom0: response write finished");
				 break;
			}

			case CHRIF_OP_READ:
			    printk(KERN_DEBUG "\nxen: dom0: CHRIF_OP_READ,then com_read");
				complete(&chrbk->com_read);
				break;

			case CHRIF_OP_IOCTL:
				if (do_ioctl_resp(chrif, req, resp))
					req->status-=1;
				break;

			case CHRIF_OP_CLOSE:
				chrbk->open_cnt--;
				//only close when the last to close
				if (chrbk->open_cnt == 0)
					filp_close(chrbk->chrif_filp, NULL);

			    make_response(chrif, req, resp);
				printk(KERN_INFO "\nxen: Dom0: response close");
				break;

			case CHRIF_OP_MAP:
				if(xen_chrbk_map(req, chrif))
					printk(KERN_INFO "\nxen: Dom0: map fronted pages fail");
				else printk(KERN_INFO "\nxen: Dom0: map fronted pages finished");
				break;
			
            default:
                printk(KERN_DEBUG "\nxen: Dom0: unknow the operation");
				req->status-=1;
				make_response(chrif, req, resp);
                break;
       }

	   free_req(pending_req);      
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
	chrbk->writed_sz = 0;
	chrbk->wting_domu_id = -1;

	init_completion(&chrbk->com_read);

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
 
