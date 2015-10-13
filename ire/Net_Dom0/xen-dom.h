/*
* xen-dom.h
* dom0 header
*/

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/rbtree.h>
#include <asm/setup.h>
#include <asm/pgalloc.h>
#include <asm/hypervisor.h>
#include <xen/grant_table.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <xen/xenbus.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/io/protocols.h>
#include "pdma-ioctl.h"
#include "xen_queue.h"


#define CHRIF_OP_WRITE 0
#define CHRIF_OP_IOCTL 1
#define CHRIF_OP_MAP 2
#define MAX_GREF 16

/*two .c file use it, so declare it here*/
#define DEVICE_PATH "/dev/pdma"
#define SHARE_MEMORY_PN 16  //64K 16page
#define PORT 2345
#define OFFSET_LENGTH sizeof(unsigned int)


struct xen_remote{ 
	char *remote_ip;
	char *remote_buf;
    unsigned int length;	
    struct socket *sock;
    struct sockaddr_in server_addr;
};

struct req_task{
    int id;
    int type;   //flag  which tpye; 0-local or 1-remote
    void *data; 
    xen_queue_element element;
    struct chrif_request *req;
    struct chrif_response *resp;
};


struct  chrif_request {
    unsigned int id;
	grant_ref_t op_gref[MAX_GREF];
    unsigned int status;
    struct pdma_stat stat;
	unsigned int operation;
	struct file *chrif_filp;
	unsigned int length;
};

struct chrif_response {
    unsigned int id;
    unsigned int status;
	struct pdma_stat stat;
    unsigned int operation;
    struct file *chrif_filp;
	unsigned int length;
};

typedef struct chrif_request chrif_request_t;
typedef struct chrif_response chrif_response_t;

DEFINE_RING_TYPES(chrif, struct chrif_request, struct chrif_response);
typedef struct chrif_sring chrif_sring_t;
typedef struct chrif_front_ring chrif_front_ring_t;
typedef struct chrif_back_ring chrif_back_ring_t;

struct xen_chrif{
    domid_t domid;
    unsigned int handle;
	unsigned int pages_handle[MAX_GREF];
    unsigned int irq;

	/* shared ring map handle */
    grant_handle_t shmem_handle;

	/* shared ring map ref */
    grant_ref_t shmem_ref;

    struct chrif_back_ring chr_ring;
	spinlock_t chr_ring_lock;
    struct vm_struct *comms_area;
	unsigned long *map_pages;

    struct backend_info *be;  
    struct file *filp;

	/* One thread per one chrif. */
	struct task_struct	*xenchrd;
	unsigned int		waiting_reqs;

	/*write buffer*/
	char *block_buf;

    /*the block info about pdma*/
    struct pdma_info pdma_info;
    struct completion comp_waitser;
};

static const char *op_name(int op)
{
    static const char *const names[] = {
        [CHRIF_OP_WRITE] = "write",
		[CHRIF_OP_IOCTL] = "ioctl",
		[CHRIF_OP_MAP] = "mapages" };

        if (op < 0 || op >= ARRAY_SIZE(names))
            return "unknown";
        if (!names[op])
            return "reserved";
        return names[op];
}

int xen_chrif_xenbus_init(void);

irqreturn_t chrif_interrupt(int irq, void *dev_id);

void xen_chrbk_unmap(struct xen_chrif *chrif);

void xen_chrif_xenbus_exit(void);

bool dev_is_on(void);

unsigned int get_send_len(unsigned int);

struct pdma_info get_pdmainfo(void);
