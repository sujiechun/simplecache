
struct _linked_list_
{
	struct _linked_list_* pre;
	struct _linked_list_* next;
};


/*
*To define a node list.
*/

#define DEFINE_CM_LINKED_LIST(p)	\
	struct p	{	\
	struct _linked_list_;	

#define END_CM_LINKED_LIST_DEFINE }

/*
*To init a list head.
*/

#define LIST_HEAD_INIT(n) {&(n),&(n)}

#define INIT_LIST_HEAD(n)	\
	(n).pre=&(n);			\
	(n).next=&(n)

#define INIT_PLIST_HEAD(ptr)	\
	(ptr)->pre=(ptr);			\
	(ptr)->next=(ptr)

#define SET_LIST_HEAD(n)	\
	struct _linked_list_ n=LIST_HEAD_INIT(n)


/*
*向链表添加节点
*/
static void _linked_list_add(struct _linked_list_* node, struct _linked_list_* pre, struct _linked_list_* next)
{
	node->pre = pre;
	node->next = next;
	pre->next = node;
	next->pre = node;
}

static void linked_list_add(struct _linked_list_* node, struct _linked_list_* head)
{
	_linked_list_add(node, head, head->next);
}

static void linked_list_add_tail(struct _linked_list_* node, struct _linked_list_* head)
{
	_linked_list_add(node, head->pre, head);
}


/*
*从链表删除节点
*/

static void _linked_list_del(struct _linked_list_* pre, struct _linked_list_* next)
{
	next->pre = pre;
	pre->next = next;
}

static void linked_list_del(struct _linked_list_* node)
{
	_linked_list_del(node->pre, node->next);
}

/*linux中使用的非常著名的宏*/
#define list_entry(ptr,type,member) \
	((type*)((char*)ptr-(unsigned long)(&((type*)0)->member)))


