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
MODULE_LICENSE("GPL");

struct gnttab_map_grant_ref ops;
struct gnttab_unmap_grant_ref unmap_ops;

struct as_request {
	unsigned int id;
        unsigned int status;
        unsigned int operation;
};
struct as_response {
        unsigned int id;
        unsigned int status;
        unsigned int operation;
};
typedef struct as_request as_request_t;
typedef struct as_response as_response_t;

DEFINE_RING_TYPES(as, struct as_request, struct as_response);
typedef struct as_sring as_sring_t;
typedef struct as_front_ring as_front_ring_t;
typedef struct as_back_ring as_back_ring_t;

struct info_t {
	int irq;
    	int gref;
    	int remoteDomain;
    	int evtchn;
    	struct as_back_ring ring;
} info;

static irqreturn_t as_int(int irq, void *dev_id)
{
	RING_IDX rc, rp;
    	as_request_t req;
    	as_response_t resp;
    	int more_to_do, notify;

    	printk(KERN_DEBUG "\nxen:Dom0: as_int called with dev_id=%x info=%x", (unsigned int)dev_id, (unsigned int) &info);
    	rc = info.ring.req_cons;
    	rp = info.ring.sring->req_prod;
    	printk(KERN_DEBUG " rc = %d rp = %d", rc, rp);
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

static int device_probe(struct xenbus_device* dev, const struct xenbus_device_id* id)
{
	struct backendinfo* binfo;
        struct vm_struct *v_start;
        int err;
        as_sring_t *sring;
	char *gref,*port;
        binfo = kmalloc(sizeof(*binfo),GFP_KERNEL);
        if (!binfo) {
               xenbus_dev_error(dev, -ENOMEM, "allocating info structure");
               return -ENOMEM;
         }
        memset(binfo,0,sizeof(*binfo));
        binfo->dev = dev;
        printk(KERN_ALERT"\nProbe fired!\n");
        
	xenbus_scanf(XBT_NIL, binfo->dev->otherend, "gref", "%u", &info.gref);
	xenbus_scanf(XBT_NIL, binfo->dev->otherend, "port", "%u", &info.evtchn);
        printk("Xenstore read port and gref success: %d, %d \n", info.evtchn, info.gref);
        info.remoteDomain = binfo->dev->otherend_id;
        printk(KERN_DEBUG "\nxen: dom0: gnt_init with gref = %d\n", info.gref);
        v_start = alloc_vm_area(PAGE_SIZE,NULL);
        if(v_start == 0){
        	free_vm_area(v_start);
           	printk("\nxen: dom0:could not allocate page");
           	return -EFAULT;
        }
        gnttab_set_map_op(&ops,(unsigned long)v_start->addr,GNTMAP_host_map,info.gref,info.remoteDomain);
       	if(HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref,&ops,1)){
       		printk(KERN_DEBUG"\nxen:dom0:HYPERVISOR map grant ref failed");
           	return -EFAULT;
       	}
       	if (ops.status) {
        	printk(KERN_DEBUG "\nxen: dom0: HYPERVISOR map grant ref failed status = %d", ops.status);
           	return -EFAULT;
       	}
       	printk(KERN_DEBUG "\nxen:dom0:shared_page=%x, handle = %x, status = %x", (unsigned int)v_start->addr, ops.handle, ops.status);

       	unmap_ops.host_addr = (unsigned long)(v_start->addr);
       	unmap_ops.handle = ops.handle;
       	sring = (as_sring_t *)v_start->addr;
       	BACK_RING_INIT(&info.ring, sring, PAGE_SIZE);
       	err = bind_interdomain_evtchn_to_irqhandler(info.remoteDomain, info.evtchn, as_int, 0, "dom0", &info);
       	if (err < 0) {
        	printk(KERN_DEBUG "\nxen:dom0: gnt_init failed binding to evtchn");
          	err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops, 1);
          	return -EFAULT;
       	}
       	info.irq = err;
       	printk(KERN_DEBUG "\nxen:dom0:end gnt_int: int = %d", info.irq);
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
    	printk(KERN_DEBUG "\nxen:dom0:gnt_exit\n");
    	ret = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops, 1);
    	if (ret == 0) {
        	printk(KERN_DEBUG "gnt_exit: unmapped shared frame\n");
    	} else {
       		printk(KERN_DEBUG "gnt_exit: unmapped shared frame failed\n");
    	}
    	xenbus_unregister_driver(&driverback);
    	printk("driver unregister success\n");
    	printk("...........................\n");
}

module_init(dom_init);
module_exit(dom_exit);





