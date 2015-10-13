#include "xen_queue.h"


MODULE_LICENSE("GPL");

//queue head init
void xen_queue_init(xen_queue_head * head){
	head->next = head;
	head->prev = head;
}


//queue init
void  xen_queue_element_init(xen_queue_element * queue_element){
	queue_element->next = queue_element;
	queue_element->prev = queue_element;
}

//queue push
void xen_queue_push(xen_queue_head *head, xen_queue_element *queue_element){
	xen_queue_element *last = (xen_queue_element *)head->prev;
	queue_element->next = head;
	queue_element->prev = last;
	last->next = queue_element;
	head->prev = queue_element;	
}

//queue pop
xen_queue_element* xen_queue_pop(xen_queue_head *head){
	xen_queue_element *first = (xen_queue_element*) head->next;
	if(first == head){
		return NULL;
	}else{
		head->next = first->next;
		((xen_queue_element *)first->next)->prev = head;
		first->next = first;
		first->prev = first;
		return first;
	}
}

//judge whether queue is empty
bool xen_queue_is_empty(xen_queue_head *head){
	return (head->next == head);
}

//queue front
xen_queue_element* xen_queue_front(xen_queue_head* head){
	if (xen_queue_is_empty(head))
	{
		return NULL;
	}else{
		return (xen_queue_element*)head->next;
	}
}

//queue length
 int xen_queue_length(xen_queue_head *head){
	int length = 0;
	xen_queue_element *element;
	if(xen_queue_is_empty(head))
		return 0;
	element = xen_queue_front(head);

	length++;
	while((xen_queue_element*)element->next != head){
			length++;
	}

	return length;
}

struct xen_chrbk *chrbk;

int xen_test_thread(void *arg){
	if(!xen_queue_is_empty(chrbk->native_queue) ){
		printk(KERN_DEBUG "\n xen_test_thread() native_queue not empty");
	}
	printk(KERN_DEBUG "\n xen_test_thread() native_queue is empty");
	
	return 0;
}


static int dom_init(void) {
	int err;
	printk(KERN_DEBUG "\n dom_init() entry");
    chrbk = kmalloc(sizeof(struct xen_chrbk), GFP_KERNEL);
    chrbk->native_queue = kmalloc(sizeof(xen_queue_element), GFP_KERNEL);
	printk(KERN_DEBUG "\n dom_init() kmalloc done");
    
    xen_queue_init(chrbk->native_queue);
	printk(KERN_DEBUG "\n dom_init() init_queue done");
	
	chrbk->thrd = kthread_run(xen_test_thread, NULL, "queue");
	if (IS_ERR(chrbk->thrd)) {
		err = PTR_ERR(chrbk->thrd);
		chrbk->thrd = NULL;   
		printk("%s\n", "start thrd thread fail");
	}
    return 0;
}


static void dom_exit(void)
{
	kthread_stop(chrbk->thrd);
	printk(KERN_DEBUG "\n don_exit() exit");
}

module_init(dom_init);
module_exit(dom_exit);

