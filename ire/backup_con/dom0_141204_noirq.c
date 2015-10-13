#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <xen/interface/xen.h>
#include <xen/xen.h>
#include <asm/xen/hypercall.h>
#include <xen/evtchn.h>
#include <xen/events.h>
#include <linux/init.h>

#include "xen-dom.h"
#include "pdma-ioctl.h"
MODULE_LICENSE("GPL");

static irqreturn_t chrif_int(int irq, void *dev_id);

struct backend_info
{
     struct xenbus_device* dev;
     struct xen_chrif *chrif;
     enum xenbus_state frontend_state;
     struct xenbus_watch hotplug_status_watch;     
};

static struct xenbus_device_id device_ids[] =
{
  {"domtest2"}, //domtest
  {""},
};

static int map_frontend_pages(struct xen_chrif *chrif, grant_ref_t ring_ref)
{
	struct gnttab_map_grant_ref op;
    gnttab_set_map_op(&op, (unsigned long)chrif->comms_area->addr, GNTMAP_host_map, ring_ref, chrif->domid);
   
	if(HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1)){
       	printk(KERN_DEBUG "\nxen: dom0: HYPERVISOR map grant ref failed");
        return -EFAULT;
    }

    if (op.status) {
		struct gnttab_unmap_grant_ref unop;
		gnttab_set_unmap_op(&unop, (unsigned long)chrif->comms_area->addr, GNTMAP_host_map, op.handle);
		HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unop, 1);
        printk(KERN_DEBUG "\nxen: dom0: op.status fail");
        return op.status;
    }

	/* record ring_ref and handle */
	chrif->shmem_ref = ring_ref;
	chrif->shmem_handle = op.handle;
    
	printk(KERN_DEBUG "\nxen: dom0: map page success, page=%x, handle = %x, status = %x", 
        (unsigned int)chrif->comms_area->addr, op.handle, op.status);
    
    printk("\nxen: dom0: map frontend pages finished,otherend_id");
	return 0;
}

static void unmap_frontend_pages(struct xen_chrif *chrif)
{
	struct gnttab_unmap_grant_ref unop;
	
	gnttab_set_unmap_op(&unop, (unsigned long)chrif->comms_area->addr, GNTMAP_host_map, chrif->shmem_handle);

	if(HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unop, 1))
		printk(KERN_DEBUG "\nxen:dom0: unmapped shared page failed");
 
	printk(KERN_DEBUG "\nxen:dom0: unmapped frontend pages finished");
}

/**
* map frontend ring page
* bind event channel
* init 
**/
static int chrif_map(struct xen_chrif *chrif, unsigned long ring_ref, unsigned int evtchn)
{
	int err;
    chrif_sring_t *sring;

	chrif->comms_area = alloc_vm_area(PAGE_SIZE, NULL);
	if(chrif->comms_area == NULL){
		free_vm_area(chrif->comms_area);
		printk("\nxen: dom0: could not allocate shared_page");
		return -ENOMEM;
	}
	
	err = map_frontend_pages(chrif, ring_ref);
	if(err){
		free_vm_area(chrif->comms_area);
        printk("\nxen: dom0: map frontend page fail");
		return err;
	}

	err = bind_interdomain_evtchn_to_irqhandler(chrif->domid, evtchn, chrif_int, 0, "dom0", chrif);
    if (err < 0) {
        printk(KERN_DEBUG "\nxen: dom0: chrif_int failed binding to evtchn");
		unmap_frontend_pages(chrif); 
		return -EFAULT;
    }

	chrif->irq = err;
    printk(KERN_DEBUG "\nxen: dom0:bind event channel fineshed: irq = %d\n", chrif->irq);

    sring = (chrif_sring_t *)chrif->comms_area->addr;
    BACK_RING_INIT(&chrif->chr_ring, sring, PAGE_SIZE);
	
    printk("\nxen: dom0: chrif map finished, otherend_id");
    return 0;
}

static int connect_ring(struct backend_info *be)
{
	struct xenbus_device *dev = be->dev;
	unsigned long ring_ref;
	unsigned int evtchn;
	int err;

	//get ring_ref, evtchn from xenStore
	err = xenbus_gather(XBT_NIL, be->dev->otherend, 
				"ring-ref", "%u", &ring_ref, 
				"event-channel", "%u", &evtchn, NULL);
	if(err){
		xenbus_dev_fatal(dev, err, "reading ring_ref, evtchn fail");
	}

    printk("\nxen: dom0: Xenstore read ring_ref,evtchn: %d, %d ",ring_ref, evtchn);
	printk("\nxen: dom0: Xenstore otherend : %s", be->dev->otherend);
    
	err = chrif_map(be->chrif, ring_ref, evtchn);

	printk("\nxen: dom0: connect ring finished,otherend_id");
	return 0;
}

static void connect(struct backend_info *be)
{
    int err;
	struct xenbus_device *dev = be->dev;

	err = connect_ring(be);
	if(err){
        printk("\nxen: dom0: connect ring fail");
		return;
    }
/*
	err = xenbus_watch_pathfmt(dev, &be->hotplug_status_watch,
			hotplug_status_changed, "%s/%s", dev->nodename,"hotplug-status");
	if(err){
		xenbus_switch_state(dev, XenbusStateConnected);
	}else{
*/

	xenbus_switch_state(dev, XenbusStateConnected);
    printk("\nxen: dom0: connect()--change state to XenbusStateConnected");

    printk("\nxen: dom0: connect finished, otherend_id ");
}

static struct xen_chrif* xen_chrif_alloc(domid_t domid)
{
	struct xen_chrif *chrif;
	chrif = kmalloc(sizeof(*chrif), GFP_KERNEL);
	chrif->domid = domid;
	spin_lock_init(&chrif->chr_ring_lock);
	init_waitqueue_head(&chrif->wq);
	init_waitqueue_head(&chrif->waiting_to_free);
	
    printk("\nxen: dom0: xen chrif alloc finished");
	
	return chrif;
}

/**
 * cancel connection
 *      unbind irq
 *      unmap ring
 **/
static void xen_chrif_disconnect(struct xen_chrif *chrif)
{
    if(chrif->irq){
        unbind_from_irqhandler(chrif->irq, chrif);
        chrif->irq = 0;
    }
    
    if(chrif->chr_ring.sring){
        xenbus_unmap_ring_vfree(chrif->be->dev, &chrif->chr_ring);
        chrif->chr_ring.sring = NULL;
    }
	
    printk("\nxen: dom0: xen chrif disconnrct finished");
}

/**
 *cancel watch
 *free chrif 
 *free backend_info
 */
static int chrback_remove(struct xenbus_device* dev)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);
/*
	if (be->hotplug_status_watch.node) {
		unregister_xenbus_watch(&be->hotplug_status_watch);
		kfree(be->hotplug_status_watch.node);
		be->hotplug_status_watch.node = NULL;
	}
*/
	if (be->chrif) {
		//xen_chrif_disconnect(be->chrif);
		kfree(be->chrif);
		be->chrif = NULL;
	}

	kfree(be);
	dev_set_drvdata(&dev->dev, NULL);
    return 0;
}


static int device_probe(struct xenbus_device* dev, const struct xenbus_device_id* id)
{
	int err;
    struct backend_info  *be;
    be = kmalloc(sizeof(*be), GFP_KERNEL);
    if (!be) {
        xenbus_dev_error(dev, -ENOMEM, "allocating backend structure");
        return -ENOMEM;
    }
    be->dev = dev;
	dev_set_drvdata(&dev->dev, be);
    printk(KERN_ALERT"\nxen: dom0: Probe fired! otherend_id: %d", dev->otherend_id);

	/**
	* write backend info to xenStore (here is nothing)
	**/

	err = xenbus_switch_state(dev, XenbusStateInitWait);
	printk("\nxen: dom0: state changed to XenbusStateInitWait");
	if(err)
		goto fail;

	be->chrif = xen_chrif_alloc(dev->otherend_id);
	if (IS_ERR(be->chrif)) {
		be->chrif = NULL;
		xenbus_dev_fatal(dev, err, "creating char interface");
		err = -1;
		goto fail;
	}

	/* setup back pointer */
	be->chrif->be = be;
	return 0;

fail:
	printk("\nxen: dom0: probe failed");
	chrback_remove(dev);
	return err;
}

    
static void frontend_changed(struct xenbus_device *dev, enum xenbus_state frontend_state)
{
    struct backend_info *be = dev_get_drvdata(&dev->dev);
    
	printk("\nxen: dom0:otherend id, frontend state: %s", xenbus_strstate(frontend_state));
    be->frontend_state = frontend_state;
    
	switch(frontend_state){
        case XenbusStateInitialising:{
            if (dev->state == XenbusStateClosed) {
                printk("\nxen: dom0: %s prepare for reconnect", dev->nodename);
                xenbus_switch_state(dev, XenbusStateInitWait);
             }
	         printk("\nxen: dom0: state changed to XenbusStateInitialising");
             break;
        }

        case XenbusStateInitialised:
	        printk("\nxen: dom0: state is XenbusStateInitiallised");

        case XenbusStateConnected:
            if (dev->state == XenbusStateConnected)
                break;
            //xen_chrif_alloc(be->chrif->domid);
            if(be->chrif)
                connect(be);
	        printk("\nxen: dom0: state changed to XenbusStateConnected");
            break;

        case XenbusStateClosing:
            xenbus_switch_state(dev, XenbusStateClosing);
	        printk("\nxen: dom0: state changed to XenbusStateClosing");
            break;

        case XenbusStateClosed:
            xen_chrif_disconnect(be->chrif);
            xenbus_switch_state(dev, XenbusStateClosed);
	        printk("\nxen: dom0: state changed to XenbusStateClosed");
            if (xenbus_dev_is_online(dev))
                break;

        case XenbusStateUnknown:
            //device_unregister(&dev->dev);
	        printk("\nxen: dom0: state Unknown");
            break;

        default:
            xenbus_dev_fatal(dev, -EINVAL, "default saw state %d at frontend", frontend_state);
            break;
    }
}

static struct xenbus_driver driverback =
{
    .driver.name = "domtest",
    .driver.owner = THIS_MODULE,
    .ids = device_ids,
    .probe = device_probe,
    .remove = chrback_remove,
    .otherend_changed = frontend_changed,
};


static irqreturn_t chrif_int(int irq, void *dev_id)
{
   /* int err;
	RING_IDX rc, rp;
    int more_to_do, notify;
    chrif_request_t req;
    chrif_response_t resp;
    struct xen_chrif *chrif = dev_id;
    printk(KERN_DEBUG "\nxen: Dom0: dev_id to xen_chrif, check the ref is %d, otherend_id", chrif->shmem_ref);
*/
    printk(KERN_INFO "\n------------------------------start response-------------------------------------");
    printk(KERN_DEBUG "\nxen: Dom0: chrif_int called with dev_id=%x ", (unsigned int)dev_id);
/*    rc = chrif->chr_ring.req_cons;
    rp = chrif->chr_ring.sring->req_prod;
    printk(KERN_DEBUG "\nxen: Dom0: rc = %d rp = %d", rc, rp);
 
	while (rc != rp) {
       if (RING_REQUEST_CONS_OVERFLOW(&chrif->chr_ring, rc))
           break;
       memcpy(&req, RING_GET_REQUEST(&chrif->chr_ring, rc), sizeof(req));
       resp.id = req.id;
       resp.operation = req.operation;
       resp.status = req.status + 1;
       printk(KERN_DEBUG "\nxen:Dom0:Recvd at IDX-%d: id = %d, op=%d, status=%d", rc, req.id, req.operation, req.status);
       chrif->chr_ring.req_cons = ++rc;
       barrier();
       switch(req.operation) {
          case 0:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = 0");
              break;
          default:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = %d", req.operation);
              break;
       }

      memcpy(RING_GET_RESPONSE(&chrif->chr_ring, chrif->chr_ring.rsp_prod_pvt), &resp, sizeof(resp));
      chrif->chr_ring.rsp_prod_pvt++;
      RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&chrif->chr_ring, notify);
      
      if (chrif->chr_ring.rsp_prod_pvt == chrif->chr_ring.req_cons) {
          RING_FINAL_CHECK_FOR_REQUESTS(&chrif->chr_ring, more_to_do);
       } else if (RING_HAS_UNCONSUMED_REQUESTS(&chrif->chr_ring)) {
          more_to_do = 1;
       }
       
       if (notify) {
          printk(KERN_DEBUG "\nxen:dom0:send notify to domu");
          notify_remote_via_irq(chrif->irq);
       }
    }
*/
    return IRQ_HANDLED;
}


static int dom_init(void) 
{
    xenbus_register_backend(&driverback);
    printk(KERN_ALERT"\n driver register success\n");
    return 0;
}

static void dom_exit(void)
{
    int err;
    printk(KERN_DEBUG "\nxen:dom0: dom_exit");
    
    xenbus_unregister_driver(&driverback);
 
    printk(KERN_INFO "\nxen:dom0: driver unregister success\n");
    printk("...........................\n");
}

module_init(dom_init);
module_exit(dom_exit);
 
