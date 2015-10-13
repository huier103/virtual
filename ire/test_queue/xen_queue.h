#include <linux/init.h>
#include <linux/kernel.h>
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
#include <xen/xenbus.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/io/protocols.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/kthread.h>

typedef struct xen_queue_ele{
	void *prev;
	void *next;
}xen_queue_element;
typedef xen_queue_element xen_queue_head;

struct xen_chrbk{
	xen_queue_head * native_queue;
	struct task_struct  *thrd;
};

 
//queue head init
void xen_queue_init(xen_queue_head * head);

//queue init
void  xen_queue_element_init(xen_queue_element * queue_element);

//queue push
void xen_queue_push(xen_queue_head *head, xen_queue_element *queue_element);

//queue pop
xen_queue_element* xen_queue_pop(xen_queue_head *head);

//judge whether queue is empty
bool xen_queue_is_empty(xen_queue_head *head);

//queue front
xen_queue_element* xen_queue_front(xen_queue_head* head);

//queue length
int xen_queue_length(xen_queue_head *head);
