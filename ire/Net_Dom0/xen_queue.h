#include <linux/init.h>
#include <linux/kernel.h>

typedef struct xen_queue_ele{
	void *prev;
	void *next;
}xen_queue_element;

typedef xen_queue_element xen_queue_head;
 
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
