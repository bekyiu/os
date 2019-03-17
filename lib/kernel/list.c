#include "list.h"
#include "interrupt.h"

void list_init(struct list* list)
{
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

//把ele插入到before之前
void list_insert_before(struct list_elem* before, struct list_elem* elem)
{
	enum intr_status old_status = intr_disable();

	before->prev->next = elem;
	elem->prev = before->prev;
	elem->next = before;
	before->prev = elem;

	intr_set_status(old_status);
}

//添加元素到队首
void list_push(struct list* plist, struct list_elem* elem)
{
	list_insert_before(plist->head.next, elem);
}

//添加元素到队尾
void list_append(struct list* plist, struct list_elem* elem)
{
	list_insert_before(&plist->tail, elem);
}

//使pelem脱离链表
void list_remove(struct list_elem* pelem)
{
	enum intr_status old_status = intr_disable();

	pelem->prev->next = pelem->next;
	pelem->next->prev = pelem->prev;
	
	intr_set_status(old_status);
}

//将链表的第一个元素弹出并返回
struct list_elem* list_pop(struct list* plist)
{
	struct list_elem* elem = plist->head.next;
	list_remove(elem);
	return elem;
}

//从链表中查找obj_elem, 成功返回true 否则false
bool elem_find(struct list* plist, struct list_elem* obj_elem)
{
	struct list_elem* elem = plist->head.next;
	while(elem != &plist->tail)
	{
		if(elem = obj_elem)
		{
			return true;
		}
		elem = elem->next;
	}
	return false;

}

//遍历链表，判断是否有符合条件的元素，条件由func来判断
struct list_elem* list_traversal(struct list* plist, function func, int arg)
{
	struct list_elem* elem = plist->head.next;
	if(list_empty(plist))
	{
		return NULL;
	}
	while(elem != &plist->tail)
	{
		if(func(elem, arg))
		{
			return elem;
		}
		elem = elem->next;
	}
	return NULL;
}

uint32_t list_len(struct list* plist)
{
	struct list_elem* elem = plist->head.next;
	uint32_t len = 0;
	while(elem != &plist->tail)
	{
		len++;
		elem = elem->next;
	}
	return len;
}

bool list_empty(struct list* plist)
{
	return (plist->head.next == &plist->tail) ? true : false;
}


