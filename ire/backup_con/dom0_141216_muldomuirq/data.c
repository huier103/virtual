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

static void make_response(struct xen_chrif *xen_chrif, unsigned int id, 
						  unsigned int operation, unsigned int status)
{
	struct xen_chrif  *chrif = xen_chrif;
	chrif_response_t resp;
	unsigned long flags;
	int notify;

	resp.id = id;
    resp.operation = operation;
    resp.status = status + 1;
       
	spin_lock_irqsave(&chrif->chr_ring_lock, flags);
	memcpy(RING_GET_RESPONSE(&chrif->chr_ring, chrif->chr_ring.rsp_prod_pvt),
		&resp, sizeof(resp));
    chrif->chr_ring.rsp_prod_pvt++;
    RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&chrif->chr_ring, notify);
	spin_unlock_irqrestore(&chrif->chr_ring_lock, flags);
       
    printk(KERN_DEBUG "\nxen:Dom0: make response id = %d, op=%d, status=%d", resp.id, resp.operation, resp.status);
	
    if (notify) {
		printk(KERN_DEBUG "\nxen:dom0:send notify to domu");
        notify_remote_via_irq(chrif->irq);
     }
}

static int __do_char_io_op(struct xen_chrif *xen_chrif)
{
	struct xen_chrif *chrif = xen_chrif;
	struct pending_req *pending_req;
    chrif_request_t req;
	RING_IDX rc, rp;    
	int more_to_do = 0;
 
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

	   memcpy(&req, RING_GET_REQUEST(&chrif->chr_ring, rc), sizeof(req));
       chrif->chr_ring.req_cons = ++rc;
       barrier();
       printk(KERN_DEBUG "\nxen:Dom0:Recvd at IDX-%d: id = %d, op=%d, status=%d", rc, req.id, req.operation, req.status);
     
	   switch(req.operation) {
          case 0:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = 0");
              break;
          default:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = %d", req.operation);
              break;
       }

	   free_req(pending_req);
       make_response(chrif, req.id, req.operation, req.status);
	}

    return 0;
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
 
