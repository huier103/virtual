#include "xen_queue.h"

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