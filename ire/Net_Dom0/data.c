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
#include <linux/sched.h>
#include <linux/time.h>

#include "xen-dom.h"
#include "pdma-ioctl.h"

MODULE_LICENSE("GPL");

static int xen_chrif_reqs = 64;
module_param_named(reqs, xen_chrif_reqs, int, 0);
MODULE_PARM_DESC(reqs, "Number of chrback requests to allocate");
mm_segment_t old_fs;
static DEFINE_MUTEX(op_pdma_lock);

int taskid = 0;
//time count
struct timespec timestar;
struct timespec timend;
struct completion recv_server_resp;


struct pending_req {
	struct xen_chrif	*chrif;
	struct list_head	free_list;
};

struct xen_chrbk {
	struct pending_req	*pending_reqs;
	/* List of all 'pending_req' available */
	struct list_head	pending_free;
	/*  its spinlock. */
	spinlock_t		pending_free_lock;
	struct   file *chrif_filp;
	/*deal with the unconstant write*/
	int wt_count;   
	/*thread deal with req*/
	struct task_struct  *reqthrd;
	struct task_struct  *conthrd;
	/*wait queue*/
	wait_queue_head_t	nativeq_wq;
	wait_queue_head_t	remoteq_wq;
	wait_queue_head_t	sentcliq_wq;
	/*global queue head*/
	xen_queue_head *native_queue;
	xen_queue_head *remote_queue;
	/*the req in queue has been sent to server*/
	xen_queue_head *submit_queue;
	/*the req in queue need to be sent to client*/
	xen_queue_head *sendtocli_queue;
	/*wether has dev*/
	bool dev_on;
	/* log remote info*/
	struct xen_remote *remote_info;
	/*used by remote buf*/
	int buf_count ; 
};

spinlock_t native_queue_lock;
spinlock_t sendtocli_queue_lock;
spinlock_t remote_queue_lock;
spinlock_t submit_queue_lock;
static struct xen_chrbk *chrbk;



/*
 * Retrieve from the 'pending_reqs' a free pending_req structure to be used.
 */
static struct pending_req *alloc_req(void)
{
	struct pending_req *req = NULL;
	unsigned long flags;
	printk(KERN_DEBUG "\nxen: Dom0: alloc_req()- entry");

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
	printk(KERN_DEBUG "\nxen: Dom0: free_req()- entry");

	spin_lock_irqsave(&chrbk->pending_free_lock, flags);
	was_empty = list_empty(&chrbk->pending_free);
	list_add(&req->free_list, &chrbk->pending_free);
	spin_unlock_irqrestore(&chrbk->pending_free_lock, flags);
}


/*
 * pdma write by driver  
 */
int pdma_write(char * buf_addr, unsigned int block_sz){
	int err;
	printk(KERN_DEBUG "\nxen: Dom0: pdma_write()- entry");

	old_fs = get_fs();
	set_fs(get_ds()); 
	err = chrbk->chrif_filp->f_op->write(chrbk->chrif_filp, buf_addr, 
							block_sz, &chrbk->chrif_filp->f_pos);	
	set_fs(old_fs);
	printk(KERN_DEBUG "\nxen: Dom0: pdma_write done");
	return err;
}


/*
 *pdma read by driver
 */
int pdma_read(char* buf_addr, unsigned int block_sz){
	int err;
	printk(KERN_DEBUG "\nxen: Dom0: pdma_read()- entry");

	old_fs = get_fs();
	set_fs(get_ds());
	err = chrbk->chrif_filp->f_op->read(chrbk->chrif_filp, buf_addr, 
										block_sz, &chrbk->chrif_filp->f_pos);	
	set_fs(old_fs);
	printk(KERN_DEBUG "\nxen: Dom0: pdma_read done");
	return err;
}


/*
 *  read/write pamd driver register
 */
 int pdma_ioctl_op(unsigned int cmd, unsigned long data_addr){
 	int err;
 	printk(KERN_DEBUG "\nxen: Dom0: pdma_ioctl_op()- entry");

 	old_fs = get_fs();
	set_fs(get_ds());
	err = chrbk->chrif_filp->f_op->unlocked_ioctl(chrbk->chrif_filp, cmd, data_addr);	
	set_fs(old_fs);
	return err;
 }

	

/*
 * map fronted wr_pages
 */
static int xen_chrbk_map(struct chrif_request *request, struct xen_chrif *xen_chrif)
{
	int i;
//    unsigned long *map_page;
    struct vm_struct *vm_area;
	struct chrif_request *req = request;
	struct xen_chrif *chrif = xen_chrif;
	struct gnttab_map_grant_ref map_ops[MAX_GREF];
    printk(KERN_DEBUG "\nxen: Dom0: xen_chrbk_map()- entry");

    vm_area = alloc_vm_area(SHARE_MEMORY_PN * PAGE_SIZE, NULL);
	for(i = 0; i < SHARE_MEMORY_PN; i++){
		chrif->map_pages[i] = (unsigned long)(vm_area->addr + i*PAGE_SIZE);
        gnttab_set_map_op(&map_ops[i], (unsigned long)chrif->map_pages[i], 
			GNTMAP_host_map, req->op_gref[i], chrif->domid);
   
		if(HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &map_ops[i], 1)){
       		printk(KERN_DEBUG "\nxen: dom0: map pages failed");
			return -EFAULT;
		}

		if (map_ops[i].status) {
			struct gnttab_unmap_grant_ref unmap_op;
			gnttab_set_unmap_op(&unmap_op, (unsigned long)chrif->map_pages[i], 
				GNTMAP_host_map, map_ops[i].handle);
			HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_op, 1);
			printk(KERN_DEBUG "\nxen: dom0: map_op.status fail");
			return map_ops[i].status;
		}

       	printk(KERN_DEBUG "\nxen: dom0: vm_area->addr[%d]: 0x%x ", 
        i, vm_area->addr+i*PAGE_SIZE);
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
	printk(KERN_DEBUG "\nxen: Dom0: xen_chrbk_unmap()- entry");

	for(i = 0; i < SHARE_MEMORY_PN; i++){
        map_page = chrif->map_pages[i];
		gnttab_set_unmap_op(&unmap_ops[i], (unsigned long)map_page, 
				GNTMAP_host_map, chrif->pages_handle[i]);
		HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops[i], 1);
	}

	//free_vm_area(chrif->map_pages);
}


static void xen_make_response(struct xen_chrif *xen_chrif, 
							  struct chrif_request *req, 
							  struct chrif_response *response)
{
	struct xen_chrif  *chrif = xen_chrif;
	struct chrif_response *resp = response;
	unsigned long flags;
	int notify;
	printk(KERN_DEBUG "\nxen: Dom0: xen_make_response()- entry");

	resp->id = req->id;
    resp->operation = req->operation;
    resp->status = req->status + 1;
	
    spin_lock_irqsave(&chrif->chr_ring_lock, flags);
    
	memcpy(RING_GET_RESPONSE(&chrif->chr_ring, chrif->chr_ring.rsp_prod_pvt), 
							resp, sizeof(*resp));
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

static int do_read_data(struct xen_chrif *xen_chrif)
{
	int err, i;
	struct xen_chrif *chrif = xen_chrif;
	struct pdma_rw_reg ctrl;
	int rd_count, length;
	int rd_block_sz = chrif->pdma_info.rd_block_sz;
	printk(KERN_DEBUG "\nxen: Dom0: do_read_data()- entry");

    ctrl.type = 0; //read
	ctrl.addr  = 0;
	err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
    printk(KERN_DEBUG "\nxen: Dom0: read form reg: %d", ctrl.val);
	if(err){
		printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
	}
	
	length = ctrl.val;
	if(length%rd_block_sz)
		rd_count = length/rd_block_sz + 1;
	else rd_count = length/rd_block_sz;
	//in case crel.val == 0
    if(rd_count == 0) rd_count = 1;
    
    /* read and send all data to frontend */
	for(i = 0; i < rd_count; i++){
		err = pdma_read((char*)(chrif->map_pages[0] + i*rd_block_sz), rd_block_sz);	
		if(err < 0){
			printk(KERN_DEBUG "\nxen: Dom0: read %u bytes error", length);
		}
	}
	
	printk(KERN_DEBUG "\nxen: Dom0: read data from pdma and put it into shared pages");
    
    //time count
    getnstimeofday(&timend);
    printk(KERN_INFO "\nxen: Dom0: use sec-%d, nsec-%d\n", 
    		timend.tv_sec-timestar.tv_sec, 
    		timend.tv_nsec-timestar.tv_nsec);

	//finish read, reset reg 
    ctrl.type = 1; //write
    ctrl.addr  = 0;
    ctrl.val = 1;
    err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
    if(err){
	    printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
    }		

	return 0;
}



/*
 *deal with native request, native take over it
 */
static int do_write_resp(struct xen_chrif *xen_chrif, 
						struct chrif_request *req, 
						struct chrif_response *resp)
{
	int i, err, code_len;
	struct pdma_rw_reg ctrl;
    struct xen_chrif *chrif = xen_chrif;   
    int wt_block_sz = chrif->pdma_info.wt_block_sz;
	printk(KERN_DEBUG "\nxen: Dom0: do_write_resp() entry ");

    //reset reg 
    ctrl.type = 1; //write
    ctrl.addr  = 0;
    ctrl.val = 1;
    err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
	//printk(KERN_DEBUG "\nxen: Dom0: reset reg------------");
    if(err){
	    printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
    }

    //account write count
    if(req->length%wt_block_sz)
		chrbk->wt_count = req->length/wt_block_sz + 1;
	else
		chrbk->wt_count = req->length/wt_block_sz;

	//schedule_timeout_uninterruptible(500);
	  
    getnstimeofday(&timestar);
        
    //write reg len
    code_len = (req->length/8) << 19;
    ctrl.val = code_len;
    err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
	printk(KERN_INFO "\nxen: Dom0: write reg :%x, byte: %d------------", 
			ctrl.val, (req->length/8-OFFSET_LENGTH)/8);
    if(err){
	    printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
    }

	//schedule_timeout_uninterruptible(500);
	/* write data from buffer to pdma device */ 
	for(i = 0; i < chrbk->wt_count; i++){
	    err = pdma_write((char*)(chrif->map_pages[0] + OFFSET_LENGTH + i*wt_block_sz), 
	    				wt_block_sz);
	    if(err < 0){
			printk(KERN_DEBUG "\nxen: Dom0: write %u bytes error", wt_block_sz);
		}
	}
    memset((char*)chrif->map_pages[0], 0, PAGE_SIZE * SHARE_MEMORY_PN);

    /*server end, read data from pama */
    if(chrbk->dev_on){
	    //read data from pdma
	    if(do_read_data(chrif))
	    	printk(KERN_DEBUG "\nxen: Dom0: read  data from pdma error");
	    else printk(KERN_DEBUG "\nxen: Dom0: read data from pdma done!");

	    //response write
		xen_make_response(chrif, req, resp);
	}
	else{ //client end, wait for receiving data from server
		wait_for_completion(&chrif->comp_waitser);
	}

	return 0;
}


/**
* not useful
**/
static int do_ioctl_resp(struct xen_chrif *chrif, 
						struct chrif_request *req, 
						struct chrif_response *resp)
{
	int err = 0;
	struct pdma_stat pdma_stat;
	printk(KERN_DEBUG "\nxen: Dom0: do_ioctl_resp()- entry");
	
	err = pdma_ioctl_op(PDMA_IOC_STAT, (unsigned long)&pdma_stat);	
	if(err){
		printk(KERN_DEBUG "\nxen: Dom0: stat ioctl failed");
	}else  printk(KERN_DEBUG "\nxen: Dom0: stat ioctl success");
	resp->stat = pdma_stat;	

	xen_make_response(chrif, req, resp);
	printk(KERN_INFO "\nxen: Dom0: response ioctl");

	return err;
}



/*
 *deal with req according to operation
 */
static int __do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	struct pending_req  *pending_req;
    struct chrif_request  *req;
	chrif_response_t  *resp;
	RING_IDX  rc, rp;    
	int more_to_do = 0;
    resp = kmalloc(sizeof(*resp), GFP_KERNEL);
    printk(KERN_DEBUG "\nxen: Dom0: __do_char_io_op()- entry");
 
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
	   
		switch(req->operation) {			
			  case CHRIF_OP_WRITE:{
					//schedule_timeout_uninterruptible(500);
					struct req_task *task;
					unsigned long flags;
					
					task = (struct req_task *)kmalloc(sizeof(struct req_task), GFP_KERNEL);
					task->data = (void *)xen_chrif;		
					task->type = 0; //native
					task->req = req;
					task->resp = resp;
					//xen_queue_element_init(&task->element);

					//server end--add to native_queue
					if(chrbk->dev_on){  
						spin_lock_irqsave(&native_queue_lock, flags);			
						xen_queue_push(chrbk->native_queue, &task->element);
						wake_up(&chrbk->nativeq_wq);
						spin_unlock_irqrestore(&native_queue_lock, flags);
						printk(KERN_INFO "\nxen: Dom0-server: add task to native queue");
					}
					else{//add to remote_queue
						spin_lock_irqsave(&remote_queue_lock, flags);			
						xen_queue_push(chrbk->remote_queue, &task->element);
						wake_up(&chrbk->remoteq_wq);
						spin_unlock_irqrestore(&remote_queue_lock, flags);
						printk(KERN_INFO "\nxen: Dom0-client: add task to remote queue");
					}
			  		break;
			 }

			case CHRIF_OP_IOCTL:
				if (do_ioctl_resp(chrif, req, resp))
					req->status-=1;
				break;
			
			case CHRIF_OP_MAP:
				if(xen_chrbk_map(req, chrif))
					printk(KERN_INFO "\nxen: Dom0: map fronted pages fail");
				else printk(KERN_INFO "\nxen: Dom0: map fronted pages finished");
				break;
			
			default:
				printk(KERN_DEBUG "\nxen: Dom0: unknow the operation");
				req->status-=1;
				xen_make_response(chrif, req, resp);
				break;
		}
	   free_req(pending_req);      
	   
	}

    return more_to_do;
}

static int do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	int more_to_do;
	printk(KERN_DEBUG "\nxen: Dom0: do_char_io_op()- entry");

	do {
		more_to_do = __do_char_io_op(chrif);
		if (more_to_do)
			break;

		RING_FINAL_CHECK_FOR_REQUESTS(&chrif->chr_ring, more_to_do);
	} while (more_to_do);

	return more_to_do;
}


/*
 *server end, write and read data, then add to sendtocli_queue 
 *response remote request
 */
static int do_resp_remote_io_op(struct xen_remote* xen_remote){
	struct xen_remote *remote = xen_remote;
	struct pdma_info pdma_info;
	struct pdma_rw_reg ctrl;
	int code_len;
	int wtcount, rdcount;
	int  i, err;
	struct req_task *task;
	unsigned long flags;
	unsigned int data_length;
	printk(KERN_DEBUG "\nxen: Dom0: do_resp_remote_io_op() entry");

	memcpy(&data_length, remote->remote_buf, sizeof(unsigned int));
	remote->length = data_length;
	pdma_info = get_pdmainfo();
	//write reg length
	code_len = (data_length/8) << 19;
    ctrl.val = code_len;
    ctrl.type = 1;   //write
    err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
	printk(KERN_INFO "\nxen: Dom0: write reg :%x, byte: %d------------", 
			ctrl.val, remote->length);
    if(err){
	    printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
    }	

    if(data_length%pdma_info.wt_block_sz)
		wtcount = data_length/pdma_info.wt_block_sz + 1;
	else
		wtcount = data_length/pdma_info.wt_block_sz;
	//write data into pdma
	for(i = 0; i < wtcount; i++){
		err = pdma_write((char *)(remote->remote_buf + OFFSET_LENGTH + 
						i*pdma_info.wt_block_sz), 
						pdma_info.wt_block_sz);	
	    if(err < 0){
			printk(KERN_DEBUG "\nxen: Dom0: write %u bytes at %d count error", 
					pdma_info.wt_block_sz, i + 1);
		}
	}

	//read reg
    ctrl.type = 0;   //read
    err = pdma_ioctl_op(PDMA_IOC_RW_REG, (unsigned long)&ctrl);	
	printk(KERN_INFO "\nxen: Dom0: read reg :%x------------", ctrl.val);
    if(err){
	    printk(KERN_DEBUG "\nxen: Dom0: rw-reg ioctl failed");
    }

    //read data from pdma
    if(ctrl.val%pdma_info.wt_block_sz)
		rdcount = ctrl.val/pdma_info.wt_block_sz + 1;
	else
		rdcount = ctrl.val/pdma_info.wt_block_sz;
	if(rdcount == 0) rdcount = 1; //in case ctrl.val == 0
	for(i = 0; i < rdcount; i++){
		err = pdma_read((char *)(remote->remote_buf + i*pdma_info.rd_block_sz), 
						pdma_info.rd_block_sz);
		if(err < 0){
			printk(KERN_DEBUG "\nxen: Dom0: read %u bytes error", ctrl.val);
		}			
	}
	printk(KERN_DEBUG "\nxen: Dom0: remote read data from pdma done");

	task = kmalloc(sizeof(struct req_task), GFP_KERNEL);
	if(taskid == 1000) taskid = 0;
	task->id = taskid++;
	task->type = 1;
	task->data = (void *)remote;
	spin_lock_irqsave(&sendtocli_queue_lock, flags);
	xen_queue_push(chrbk->sendtocli_queue, &task->element);
	spin_unlock_irqrestore(&sendtocli_queue_lock, flags);
	wake_up(&chrbk->sentcliq_wq);

	return 0; 
}



/**
* called when native request comes
* add req to queue
**/
irqreturn_t chrif_interrupt(int irq, void *dev_id)
{
	printk(KERN_INFO "\n------------------------------start response-------------------------------------");
    printk(KERN_DEBUG "\nxen: Dom0: chrif_intterrupt() entry: dev_id=%x ", (unsigned int)dev_id);
	if(do_char_io_op((struct xen_chrif *)dev_id))
		printk(KERN_DEBUG "\nxen: Dom0: response finished");
	else printk(KERN_DEBUG "\nxen: Dom0: deal with response failed");

	return IRQ_HANDLED;
}



/**
* server end
* receive req from remote, add it to native_queue
**/
void xen_sock_recvmsg_from_client(void ){
	int len;
	struct req_task *task;
	unsigned long flags_native;
	struct kvec recv_vec;
	struct msghdr recv_msg;
	struct xen_remote *remote_info = chrbk->remote_info;
	printk(KERN_DEBUG "\nxen: Dom0: xen_sock_recvmsg_from_client()- entry");

	memset(&recv_vec, 0, sizeof(recv_vec));
	memset(&recv_msg, 0, sizeof(recv_msg));
	recv_vec.iov_base = (void *)remote_info->remote_buf;
	recv_vec.iov_len = (__kernel_size_t)SHARE_MEMORY_PN * PAGE_SIZE;
	recv_msg.msg_name = &(remote_info->server_addr);
	recv_msg.msg_namelen = sizeof(remote_info->server_addr);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = kernel_recvmsg(remote_info->sock, 
						&recv_msg, &recv_vec, 1, 
						SHARE_MEMORY_PN*PAGE_SIZE, 0);
	set_fs(old_fs);
	if(len < 0)
		printk("\n xen: Dom0-server: recv msg fail len-%d ", len);
	else printk("\n xen: Dom0-server: recv msg %d success ", len);

	task = kmalloc(sizeof(struct req_task), GFP_KERNEL);
	//recv msg, added to native_queue, remote to local
	if(taskid == 1000) taskid = 0;
	task->id = taskid++;
	task->type = 1;	//remote task
	task->data = (void *)remote_info;
	spin_lock_irqsave(&native_queue_lock, flags_native);
	xen_queue_push(chrbk->native_queue, &task->element);
	wake_up(&chrbk->nativeq_wq);
	spin_unlock_irqrestore(&native_queue_lock, flags_native);
}


/**
 * server end 
 * socket send message to client end
 **/
static int xen_sock_sendmsg_to_client(struct xen_remote *remote, 
									  struct req_task *reqtask)
{
	struct req_task *task = reqtask;
	struct xen_remote *remote_info = remote;
	struct kvec send_vec;
	struct msghdr send_msg;
	int len;
	unsigned int send_len;
	printk(KERN_DEBUG "\nxen: Dom0: xen_sock_sendmsg_to_client()- entry");

	send_len = get_send_len(remote_info->length);
	memset(&send_vec, 0, sizeof(send_vec));
	memset(&send_msg, 0, sizeof(send_msg));
	send_vec.iov_base = (void *)remote_info->remote_buf;
	send_vec.iov_len = (__kernel_size_t)send_len;
	send_msg.msg_name = &(remote_info->server_addr);
	send_msg.msg_namelen = sizeof(remote_info->server_addr);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = kernel_sendmsg(remote_info->sock, &send_msg, &send_vec, 1, send_len);
    set_fs(old_fs);
    if(len < 0)
		printk("\n xen: Dom0-server: send msg fail len-%d ", len);
	else printk("\n xen: Dom0-server: send msg %d success ", len);

	kfree(task);
    return len;
}


/**------------------------------no done-------------------------------
 * client end
 * receive data has been decoded from server
 **/
static int xen_sock_recvmsg_from_server(void ){
	int len;
	struct req_task *task;
	struct xen_chrif *chrif;
	unsigned long flags;
	xen_queue_element* rtnelement;
	struct kvec recv_vec;
	struct msghdr recv_msg;
	struct xen_remote *remote_info = chrbk->remote_info;
	printk(KERN_DEBUG "\nxen: Dom0: xen_sock_recvmsg_from_server()- entry");

	spin_lock_irqsave(&submit_queue_lock, flags);
	rtnelement = xen_queue_pop(chrbk->submit_queue);
	spin_unlock_irqrestore(&submit_queue_lock, flags);
	task = container_of(rtnelement, struct req_task, element);
	chrif = (struct xen_chrif *)task->data;

	memset(&recv_vec, 0, sizeof(recv_vec));
	memset(&recv_msg, 0, sizeof(recv_msg));
	recv_vec.iov_base = (void *)chrif->map_pages[0];
	recv_vec.iov_len = (__kernel_size_t)SHARE_MEMORY_PN * PAGE_SIZE;
	recv_msg.msg_name = &(remote_info->server_addr);
	recv_msg.msg_namelen = sizeof(struct sockaddr_in);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = kernel_recvmsg(remote_info->sock, 
						&recv_msg, &recv_vec, 1, 
						SHARE_MEMORY_PN*PAGE_SIZE, 0);
	set_fs(old_fs);
	if(len < 0)
		printk("\n xen: Dom0-client: recv msg fail len-%d ", len);
	else printk("\n xen: Dom0-client: recv msg %d success ", len);
	
	complete(&recv_server_resp);
	complete(&chrif->comp_waitser);

	return len;
}


/**
 * client end
 * socket send message to server end 
 * msg == encodelen + data
 */
static int xen_sock_sendmsg_to_server(struct req_task *reqtask){
	struct kvec send_vec;
	struct msghdr send_msg;
	int len;
	struct req_task *task = reqtask;
	struct xen_chrif *chrif = (struct xen_chrif *)task->data;
	struct xen_remote *remote_info = chrbk->remote_info;
	printk(KERN_DEBUG "\nxen: Dom0: xen_sock_sendmsg_to_server()- entry");

	memset(&send_vec, 0, sizeof(send_vec));
	memset(&send_msg, 0, sizeof(send_msg));
	memcpy(chrif->map_pages, &task->req->length, sizeof(unsigned int));
	send_vec.iov_base = (char *) (chrif->map_pages[0] + OFFSET_LENGTH);
	send_vec.iov_len = (__kernel_size_t)task->req->length;
	send_msg.msg_name = &(remote_info->server_addr);
	send_msg.msg_namelen = sizeof(struct sockaddr_in);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = kernel_sendmsg(remote_info->sock, 
						&send_msg, &send_vec, 1, 
						task->req->length + OFFSET_LENGTH);
	set_fs(old_fs);
	if(len < 0)
		printk("\n xen: Dom0-client: send msg fail len-%d ", len);
	else printk("\n xen: Dom0-client: send msg %d success ", len);
	
    return len;
}



/**
* dispatch native queue
**/
void dispath_native_queue(void){
	struct req_task *task;
	struct xen_chrif *chrif;
	struct xen_remote *remote_info;
	xen_queue_element* queue_element;
	unsigned long flags;

	wait_event_interruptible(chrbk->nativeq_wq,
							!xen_queue_is_empty(chrbk->native_queue)||
							kthread_should_stop());	
	spin_lock_irqsave(&native_queue_lock, flags);
	queue_element = xen_queue_pop(chrbk->native_queue);
	spin_unlock_irqrestore(&native_queue_lock, flags);

	task = container_of(queue_element, struct req_task, element);
	if(task->type == 0){ //native request		
		chrif = (struct xen_chrif *)task->data;
		mutex_lock(&op_pdma_lock);
		if(do_write_resp(chrif, task->req, task->resp))
			printk(KERN_DEBUG "\nxen: dom0: response write fail");
		else printk(KERN_DEBUG "\nxen: dom0: response write finished");
		mutex_unlock(&op_pdma_lock);
	}
	else{   //remote request
		remote_info = (struct xen_remote *)task->data;
		do_resp_remote_io_op(remote_info);
	}
	
	printk(KERN_DEBUG "\nxen: dom0-server:dispath_native_queue()- queue empty");
	
}


/**
* server end
* dispatch sendtocli_queue,send the task has been proposed to client
* this design of queue suit for multiple no_dev_dom0, however
* design of xen_remote->remote_buf just suit for one no_dev_dom0  
**/
void dispatch_sendtocli_queue(void){
	struct req_task *task;
	unsigned long flags;
	xen_queue_element* sendelement;
	struct xen_remote *remote_info = chrbk->remote_info;
	printk(KERN_DEBUG "\nxen: Dom0: dispatch_sendtocli_queue()- entry");

	wait_event_interruptible(chrbk->sentcliq_wq,
							!xen_queue_is_empty(chrbk->sendtocli_queue)||
							kthread_should_stop());
	spin_lock_irqsave(&sendtocli_queue_lock, flags);
	sendelement = xen_queue_pop(chrbk->sendtocli_queue);
	spin_unlock_irqrestore(&sendtocli_queue_lock, flags);
	
	task = container_of(sendelement, struct req_task, element);
	//according to connection, send data to client
	xen_sock_sendmsg_to_client(remote_info, task);
	
}

/**
* client end
* pop remote_queue element, send req to server-end's native_queue
* push req to submit_queue so as to recode the info
**/
void dispatch_remote_queue(void){
	unsigned long flags_remote;
	unsigned long flags_submit;
	struct req_task *task;
	xen_queue_element* ntrelement;
	printk(KERN_DEBUG "\nxen: Dom0: dispath_remote_queue()- entry");

	/*deal with remote_queue, local to remote*/	
	wait_event_interruptible(chrbk->remoteq_wq,
							 !xen_queue_is_empty(chrbk->remote_queue)||
							  kthread_should_stop());
	spin_lock_irqsave(&remote_queue_lock, flags_remote);
	ntrelement = xen_queue_pop(chrbk->remote_queue);
	spin_unlock_irqrestore(&remote_queue_lock, flags_remote);
	task = container_of(ntrelement, struct req_task, element);

	//get data from DomU
	do_write_resp((struct xen_chrif *)task->data, task->req, task->resp);
	xen_sock_sendmsg_to_server(task);
	//do_char_io_op((xen_chrif *)task->data, remote_info);
	//could get req info after get response.
	spin_lock_irqsave(&submit_queue_lock, flags_submit);
	xen_queue_push(chrbk->submit_queue, &task->element);
	spin_unlock_irqrestore(&submit_queue_lock, flags_submit);

	//receive resp from server, later change, only suit for send a req
	xen_sock_recvmsg_from_server();
	wait_for_completion(&recv_server_resp);
}



/**
*thread dispath all req from differ DomU
*/
int xen_req_thread(void *arg){
	printk(KERN_DEBUG "\nxen: Dom0: dispath_native_queue()- will entry");
	while(1){
		dispath_native_queue();
	}
	return 0;
}



/**
*thread connet other dom0
*/
int xen_remote_thread(void *arg){
	int addr_len;
	int ret;		
	struct xen_remote *remote_info = chrbk->remote_info;
	printk(KERN_DEBUG "\nxen: Dom0: xen_remote_thread()- entry");

	addr_len = sizeof(struct sockaddr_in);

	memset(&remote_info->server_addr, 0, addr_len);
	remote_info->server_addr.sin_family = AF_INET;
	remote_info->server_addr.sin_port = htons(PORT);
	if(chrbk->dev_on){  //we are server
		remote_info->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		printk("\n xen:Dom0 local is server end ");
	}else{
		remote_info->server_addr.sin_addr.s_addr = in_aton("11.11.11.1");
		printk("\nxen:Dom0 local is client end");
	}
	
	remote_info->sock = (struct socket *)kmalloc(sizeof(struct socket), GFP_KERNEL);
	/*create a socket*/
	ret = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &remote_info->sock);
	if(ret){
		printk("\n sever: socket_create error!");
		//return ret;
	}else{
		printk("\n server: socket_create ok!");
	}

	if(chrbk->dev_on){ //server end
		ret = kernel_bind(remote_info->sock, (struct sockaddr *)&remote_info->server_addr, addr_len);
		if(ret < 0){
			printk("server: bind error!\n");
			//return ret;
		}else{
			printk("server: bind ok!\n");
		}
	}

	if(chrbk->dev_on){  //server end
		while(1){
			xen_sock_recvmsg_from_client();
		}
	}else{      //client end
		while(1){
			dispatch_remote_queue();
		}
	}	 	
}


/******************tool func*****************/
//judge wether own dev
bool dev_is_on(void){
	return chrbk->dev_on;
}


//get length need to send to client by computing 
unsigned int get_send_len(unsigned int length){
	unsigned int send_len, code_byte;
	unsigned int encodelen = length;
	code_byte = (encodelen - 32)/64;  
	if(code_byte%4) code_byte = code_byte/4 + 1;
	else code_byte = code_byte/4;
	send_len = code_byte*8;
	return send_len;
}


/*****************tool func******************/

static int dom_init(void) 
{
    int i;
	int err = 0;
	char *req_name = "reqthread";
	char *con_name = "conthread";
	printk(KERN_DEBUG "\nxen: Dom0: dom_init()- entry");
    
    chrbk = kzalloc(sizeof(struct xen_chrbk), GFP_KERNEL);
    if(!chrbk){
        printk(KERN_DEBUG "\nxen: dom0: out of memory when alloc chrbk");
    }

	chrbk->pending_reqs = kzalloc(sizeof(chrbk->pending_reqs[0]) *xen_chrif_reqs, GFP_KERNEL);
	chrbk->remote_info = kmalloc(sizeof(struct xen_remote), GFP_KERNEL);
	//need to change to connect multiple no_dev_dom0
	chrbk->remote_info->remote_buf = vmalloc(SHARE_MEMORY_PN * PAGE_SIZE);

	INIT_LIST_HEAD(&chrbk->pending_free);
	spin_lock_init(&chrbk->pending_free_lock);

	//open dev
	chrbk->chrif_filp = filp_open(DEVICE_PATH, O_RDWR, 0);
	//no dev
	if(chrbk->chrif_filp == NULL){
		chrbk->dev_on = false;
		printk("\nxen Dom0: it is client end");
	}else {
		chrbk->dev_on = true;
		printk("\n xen Dom0: it is server end");
	}

	if(chrbk->dev_on){ //server end
		chrbk->native_queue = kmalloc(sizeof(xen_queue_element), GFP_KERNEL);
		chrbk->sendtocli_queue = kmalloc(sizeof(xen_queue_element), GFP_KERNEL);
		xen_queue_init(chrbk->native_queue);
		xen_queue_init(chrbk->sendtocli_queue);
		init_waitqueue_head(&chrbk->nativeq_wq);
		init_waitqueue_head(&chrbk->sentcliq_wq);
	}
	else{//client end
		chrbk->remote_queue = kmalloc(sizeof(xen_queue_element), GFP_KERNEL);
		chrbk->submit_queue = kmalloc(sizeof(xen_queue_element), GFP_KERNEL);
		xen_queue_init(chrbk->remote_queue);
		xen_queue_init(chrbk->submit_queue);
		init_waitqueue_head(&chrbk->remoteq_wq);
	}
	
    init_completion(&recv_server_resp);

	chrbk->wt_count = 0;

	for (i = 0; i < xen_chrif_reqs; i++)
		list_add_tail(&chrbk->pending_reqs[i].free_list, &chrbk->pending_free);
    
	//init
	err = xen_chrif_xenbus_init();   
    if(err)
		goto failed_init;

	//create thread to deal with all req from different domU
	if(chrbk->dev_on){ //server end
		chrbk->reqthrd = kthread_run(xen_req_thread, NULL, req_name);
		if (IS_ERR(chrbk->reqthrd)) {
			err = PTR_ERR(chrbk->reqthrd);
			chrbk->reqthrd = NULL;   
			printk("%s\n", "start reqthrd thread fail");
		}
	}

	//connect other dom0 thread
	chrbk->conthrd = kthread_run(xen_remote_thread, NULL, con_name);

	return 0;

failed_init:
	kfree(chrbk->pending_reqs);
	kfree(chrbk);
	chrbk = NULL;
	return err;
}


static void dom_exit(void)
{
	printk(KERN_DEBUG "\nxen: Dom0: dom_exit()- entry");
	//close dev
	if(chrbk->dev_on)
		filp_close(chrbk->chrif_filp, NULL);
	//stop thread
	kthread_stop(chrbk->reqthrd);
	kthread_stop(chrbk->conthrd);
    xen_chrif_xenbus_exit();
}

module_init(dom_init);
module_exit(dom_exit);
 
