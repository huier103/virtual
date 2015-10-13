#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <xen/grant_table.h>
#include <xen/interface/io/blkif.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/xen.h>
#include <xen/xen.h>
#include <linux/vmalloc.h>
#include <asm/xen/hypervisor.h>
#include <asm/xen/hypercall.h>
#include <xen/evtchn.h>
#include <xen/events.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <xen/xenbus.h>
#include "xen-dom.h"
#include "pdma-ioctl.h"
MODULE_LICENSE("GPL");

struct gnttab_map_grant_ref ops;
struct gnttab_unmap_grant_ref unmap_ops;

typedef struct chrif_request chrif_request_t;
typedef struct chrif_response chrif_response_t;

DEFINE_RING_TYPES(chrif, struct chrif_request, struct chrif_response);
typedef struct chrif_sring chrif_sring_t;
typedef struct chrif_front_ring chrif_front_ring_t;
typedef struct chrif_back_ring chrif_back_ring_t;

struct info_t {
	int irq;
    grant_ref_t ring_gref;
    int remoteDomain;
    int evtchn;
	grant_ref_t op_gref;
	struct file *chrif_filp;
    struct chrif_back_ring ring;
} info;

static irqreturn_t chrif_int(int irq, void *dev_id)
{
    int err;
	RING_IDX rc, rp;
    int more_to_do, notify;
    chrif_request_t req;
    chrif_response_t resp;
	printk(KERN_INFO "\n------------------------------start response-------------------------------------");
    printk(KERN_DEBUG "\nxen: Dom0: chrif_int called with dev_id=%x info=%x", (unsigned int)dev_id, (unsigned int) &info);
    rc = info.ring.req_cons;
    rp = info.ring.sring->req_prod;
    printk(KERN_DEBUG "\nxen: Dom0: rc = %d rp = %d", rc, rp);
 
	while (rc != rp) {
       if (RING_REQUEST_CONS_OVERFLOW(&info.ring, rc))
           break;
       memcpy(&req, RING_GET_REQUEST(&info.ring, rc), sizeof(req));
       resp.id = req.id;
       resp.operation = req.operation;
       resp.status = req.status + 1;
       printk(KERN_DEBUG "\nxen:Dom0:Recvd at IDX-%d: id = %d, op=%d, status=%d", rc, req.id, req.operation, req.status);
       info.ring.req_cons = ++rc;
       barrier();
       switch(req.operation) {
          case 0:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = 0");
              break;
          default:
              printk(KERN_DEBUG "\nxen:dom0:req.operation = %d", req.operation);
              break;
       }
      memcpy(RING_GET_RESPONSE(&info.ring, info.ring.rsp_prod_pvt), &resp, sizeof(resp));
      info.ring.rsp_prod_pvt++;
      RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&info.ring, notify);
      if (info.ring.rsp_prod_pvt == info.ring.req_cons) {
          RING_FINAL_CHECK_FOR_REQUESTS(&info.ring, more_to_do);
       } else if (RING_HAS_UNCONSUMED_REQUESTS(&info.ring)) {
          more_to_do = 1;
       }
       if (notify) {
          printk(KERN_DEBUG "\nxen:dom0:send notify to domu");
          notify_remote_via_irq(info.irq);
       }
    }
    return IRQ_HANDLED;
}

static struct xenbus_device_id device_ids[] =
{
  {"domtest2"}, //domtest
  {""},
};

struct backendinfo
{
     struct xenbus_device* dev;
     long int frontend_id;
     struct xenbus_watch backend_watch;
     struct xenbus_watch watch;
     char* frontpath;
};

static struct vm_struct*  map_frontend_pages(grant_ref_t gref)
{
	struct vm_struct *vm_point;
	vm_point = alloc_vm_area(PAGE_SIZE,NULL);
    if(vm_point == 0){
        free_vm_area(vm_point);
        printk("\nxen: dom0: could not allocate shared_page");
        return -EFAULT;
    }
    gnttab_set_map_op(&ops, (unsigned long)vm_point->addr, GNTMAP_host_map, gref, info.remoteDomain);
    if(HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &ops, 1)){
       	printk(KERN_DEBUG "\nxen: dom0: HYPERVISOR map grant ref failed");
        return -EFAULT;
    }
    if (ops.status) {
        printk(KERN_DEBUG "\nxen: dom0: HYPERVISOR map grant ref failed status = %d", ops.status);
        return -EFAULT;
    }
    printk(KERN_DEBUG "\nxen: dom0: map shared page success, shared_page=%x, handle = %x, status = %x", (unsigned int)vm_point->addr, ops.handle, ops.status);
	return vm_point;
}

static int connect(void )
{
    int err;
    chrif_sring_t *sring;
    struct vm_struct *v_start;
    //map the granted page
	v_start = map_frontend_pages(info.ring_gref);
	//init back ring and bing event channel
    unmap_ops.host_addr = (unsigned long)(v_start->addr);
    unmap_ops.handle = ops.handle;
    sring = (chrif_sring_t *)v_start->addr;
    BACK_RING_INIT(&info.ring, sring, PAGE_SIZE);
    err = bind_interdomain_evtchn_to_irqhandler(info.remoteDomain, info.evtchn, chrif_int, 0, "dom0", &info);
    if (err < 0) {
        printk(KERN_DEBUG "\nxen: dom0: gnt_init failed binding to evtchn");
        err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops, 1);
        return -EFAULT;
    }
    info.irq = err;
	printk(KERN_DEBUG "\nxen: dom0: irq = %d\n", info.irq);
	return 0;
}

static int device_probe(struct xenbus_device* dev, const struct xenbus_device_id* id)
{
    struct backendinfo* be;
    be = kmalloc(sizeof(*be),GFP_KERNEL);
    if (!be) {
        xenbus_dev_error(dev, -ENOMEM, "allocating info structure");
        return -ENOMEM;
    }
    memset(be, 0, sizeof(*be));
    be->dev = dev;
    
    printk(KERN_ALERT"\nxen: dom0: Probe fired!");
    //get ring_gref, op_gref and port by xenStore
	xenbus_scanf(XBT_NIL, be->dev->otherend, "ring_gref", "%u", &info.ring_gref);
	xenbus_scanf(XBT_NIL, be->dev->otherend, "port", "%u", &info.evtchn);
    printk("\nxen: dom0: Xenstore read port, ring_gref and success: %d, %d , %d", info.evtchn, info.ring_gref);
	printk("\nxen: dom0: Xenstore node: %s", be->dev->otherend);
    
	info.remoteDomain = be->dev->otherend_id;
	printk("\nxen: dom0: remoteDomid: %d", dev->otherend_id);
	connect();
    return 0;
}

static int device_remove(struct xenbus_device* dev)
{
        return 0;
}

static struct xenbus_driver driverback =
{
    .driver.name = "domtest",
    .driver.owner = THIS_MODULE,
    .ids = device_ids,
    .probe = device_probe,
    .remove = device_remove,
};

static int dom_init(void) 
{
    xenbus_register_backend(&driverback);
    printk(KERN_ALERT"\n driver register success\n");
    return 0;
}

static void dom_exit(void)
{
    int ret;
    printk(KERN_DEBUG "\nxen:dom0:gnt_exit");
    ret = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops, 1);
    if (ret == 0) {
       printk(KERN_DEBUG "\nxen:dom0: gnt_exit: unmapped shared frame");
    } else {
       printk(KERN_DEBUG "\nxen:dom0: gnt_exit: unmapped shared frame failed");
    }
    xenbus_unregister_driver(&driverback);
    printk(KERN_INFO "\nxen:dom0: driver unregister success\n");
    printk("...........................\n");
}

module_init(dom_init);
module_exit(dom_exit);
 
