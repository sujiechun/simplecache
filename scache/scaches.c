#include<stdlib.h>
#include<string.h>
#include"linked_list.h"
#include"scache.h"

#define DEFAULT_MAX_RD 1000000
#define DEFAULT_MIN_RD 256
#define DEFAULT_MAX_RC_NODE 66
#define DEFAULT_MAX_RC_SIZE 2000000



struct _scaches_node_
{
	struct _linked_list_ list;
	char* cache;	
	int size;	//缓存的大小
	int s;		//数据段起点
	int e;		//数据段的终点
};

struct _recycled_caches_
{
	struct _linked_list_* caches;
	int caches_count;
	int total_rc_size;
	int max_count;
	int max_size;
};

struct _scaches_
{
	struct _linked_list_* caches;	/*缓存链表的head*/
	int nodes_count;
	int dat_size;
	struct _scaches_config_ config;
	struct _recycled_caches_* rc_caches;
};

struct _recycled_caches_ g_cache_recycle_pool;

void scache_module_init()
{
	memset(&g_cache_recycle_pool, 0, sizeof(struct _recycled_caches_));
	g_cache_recycle_pool.max_count = DEFAULT_MAX_RC_NODE;
	g_cache_recycle_pool.max_size = DEFAULT_MAX_RC_SIZE;
}

int is_node_empty(struct _scaches_node_* cache)
{
	return cache->e == cache->s;
}

struct _scaches_node_* create_cache_node(int size)
{
	struct _scaches_node_* cache;
	cache = (struct _scaches_node_*)malloc(sizeof(struct _scaches_node_));
	memset(cache, 0, sizeof(struct _scaches_node_));
	cache->cache = (char*)malloc(size);
	cache->size = size;
	return cache;
}

void free_cache_node(struct _scaches_node_* cache)
{
	if (cache==0)
	{
		return;
	}
	if (cache->cache)
	{
		free(cache->cache);
	}
	free(cache);
}

scache* new_scache(scache_config* config, recycle_pool* pool)
{
	scache* cache;
	cache = (scache*)malloc(sizeof(scache));
	memset(cache, 0, sizeof(scache));
	memcpy(&cache->config, config, sizeof(config));

	if (cache->config.max_rd == 0)
	{
		cache->config.max_rd = DEFAULT_MAX_RD;
	}
	if (cache->config.min_rd == 0)
	{
		cache->config.min_rd = DEFAULT_MIN_RD;
	}
	if (pool==0)
	{
		cache->rc_caches = &g_cache_recycle_pool;
	}
	return cache;
}

int node_recycle(struct _scaches_* caches, struct _scaches_node_* cache)
{
	if (caches->rc_caches == 0)
	{
		return 0;
	}
	if (caches->rc_caches->caches_count<caches->rc_caches->max_count)
	{
		return 0;
	}
	if (cache->size + caches->rc_caches->total_rc_size<caches->rc_caches->max_count)
	{
		return 0;
	}
	linked_list_add(caches->rc_caches->caches, &cache->list);
	caches->rc_caches->caches_count++;
	caches->rc_caches->total_rc_size += cache->size;
	return 1;
}

int read_cache_i(struct _scaches_node_* cache, char* buf, int pos, int size)
{
	int s;
	if (cache->e - cache->s + 1<size)
	{
		return -1;
	}
	s = cache->s + pos;
	memcpy(buf, cache->cache + s, size);
	return size;
}

char* read_cache(struct _scaches_node_* cache, int pos, int size)
{
	char* buf;
	if (cache->e - cache->s + 1<size)
	{
		return -1;
	}
	buf = (char*)malloc(size);
	if (read_cache_i(cache,buf,pos,size)!=size)
	{
		free(buf);
	}
	return buf;
}

void delete_node(struct _scaches_* caches, struct _scaches_node_* cache)
{
	linked_list_del(&cache->list);
	if (node_recycle(caches,cache)==0)
	{
		free_cache_node(cache);
	}
}

/*
*从回收池取出合适的node.
*/
struct _scaches_node_* retrive_cache_node(struct _recycled_caches_* pool, int size)
{
	struct _linked_list_* node;
	struct _scaches_node_* cache;

	int retrived_size = 0;
	int i = 0;
	if (pool->caches_count==0||pool->total_rc_size==0)
	{
		/*回收池有问题了！*/
		return 0;
	}

	/*取回node的算法需要重新设计*/
	node = pool->caches;
	for ( i = 0; i < pool->caches_count; i++)
	{
		cache = list_entry(node, struct _recycled_caches_, caches);
		if (cache->size>=size)
		{
			return cache;
		}
		node = node->next;
	}
	return 0;
}

struct _scaches_node_* claim_cache_node(struct _scaches_* caches, int size, int rd)
{
	int f_size;
	int rd_size;
	struct _scaches_node_* o_cache=0;

	if (rd)
	{
		if (caches->config.ard>0)
		{
			rd_size = (int)size*caches->config.ard;
			rd_size = rd_size > caches->config.min_rd ? rd_size : caches->config.min_rd;
		}
		else if (caches->config.min_rd>0)
		{
			rd_size = caches->config.min_rd;
		}
		else
		{
			rd_size = 0;
		}
		if (rd_size>caches->config.max_rd)
		{
			rd_size = caches->config.max_rd;
		}
		f_size = size + rd_size;
	}
	else
	{
		f_size = size;
	}

	/*优先从回收cache列表中获取cache*/
	if (caches->rc_caches)
	{
		o_cache= retrive_cache_node(caches->rc_caches, f_size);
	}
	if (o_cache==0)
	{
		o_cache = create_cache_node(f_size);
	}
	return o_cache;
}

char* caches_read(struct _scaches_* caches, int pos, int size)
{
	char* o_buf,t_buf;
	int i,j;
	int t_len=0;
	int s_pos;
	int err = 0;
	struct _scaches_node_* node;
	struct _linked_list_* c_list;

	int r_len;		/*节点中剩余的数据长度*/
	int len_left;	/*剩余需要读取的数据长度*/

	if (size<(caches->dat_size - pos))
	{
		return 0;
	}

	o_buf = (char*)malloc(size);
	c_list = caches->caches;

	/*获得所读取的节点和读取点在节点中的偏移*/
	for ( i = 0; i < caches->nodes_count; i++)
	{
		node = list_entry(c_list, struct _scaches_, caches);
		t_len += (node->e - node->s+1);
		if (t_len>pos)
		{
			s_pos = pos - (t_len - (node->e - node->s + 1));
			break;
		}
		c_list = c_list->next;
	}

	if (i==caches->nodes_count)
	{
		err = -1;
		goto _end_;
	}

	/*从各个节点中copy出数据*/
	len_left = size;

	while (len_left!=0) //????
	{
		r_len = node->e - node->s + 1;
		if (r_len>len_left)
		{
			t_buf = read_cache(node, 0, len_left);
			memcpy(o_buf + size - len_left, t_buf, len_left);
			len_left = 0;
			free(t_buf);
		}
		else
		{
			t_buf = read_cache(node, 0, r_len);
			memcpy(o_buf + size - len_left, t_buf, r_len);
			len_left -= r_len;
			free(t_buf);
		}
		node = list_entry(node->list.next, struct _scaches_, caches);
	}
	caches->dat_size -= size;

_end_:
	if (err!=0&&o_buf!=0)
	{
		free(o_buf);
		o_buf = 0;
	}
	return o_buf;
}

int caches_delete(struct _scaches_* caches, int pos, int size)
{
	char* t_buf;
	int i;
	int t_len = 0;
	int s_pos;
	int err = 0;
	struct _scaches_node_* node;
	struct _linked_list_* c_list;

	int len_left;


	if (pos+size>caches->dat_size)
	{
		return -1;
	}

	/*获得所操作的节点和操作点在节点中的偏移*/
	for (i = 0; i < caches->nodes_count; i++)
	{
		node = list_entry(c_list, struct _scaches_, caches);
		t_len += (node->e - node->s + 1);
		if (t_len>pos)
		{
			s_pos = pos - (t_len - (node->e - node->s + 1));
			break;
		}
		c_list = c_list->next;
	}

	len_left = size;
	for (i = 0; len_left!=0; )
	{

	}
}

int insert_node(struct _scaches_* caches, int size)
{

}

int caches_write(struct _scaches_* caches, char* data, int size)
{

}

int caches_insert(struct _scaches_* caches, int pos, char* data, int size)
{
	char* t_buf;
	int i;
	int t_len = 0;
	int s_pos;
	struct _scaches_node_* node;
	struct _linked_list_* c_list;

	int claim_size;
	struct _scaches_node_* new_node;

	if (caches->dat_size<pos)
	{
		return 1;
	}

	if (caches->nodes_count==0)
	{
		new_node = claim_cache_node(caches, size, 1);
		if (new_node==0)
		{
			return 1;
		}
		new_node->list.next = &new_node->list;
		new_node->list.pre = &new_node->list;
		caches->caches = new_node;
		caches->nodes_count++;
	}

	c_list = caches->caches;

	/*获得所操作的节点和操作点在节点中的偏移*/
	for (i = 0; i < caches->nodes_count; i++)
	{
		node = list_entry(c_list, struct _scaches_, caches);
		t_len += (node->e - node->s + 1);
		if (t_len>pos)
		{
			s_pos = pos - (t_len - (node->e - node->s + 1));
			break;
		}
		c_list = c_list->next;
	}

	/*申请一个新的节点，由于是插入操作，申请节点是不需要冗余的*/
	claim_size = ((node->e - node->s) - s_pos) + size;
	new_node = claim_cache_node(caches, claim_size, 0);
	if (new_node == 0)
	{
		return -1;
	}
	linked_list_add(&new_node->list, &node->list);

	/*拷贝数据*/
	memcpy(new_node->cache, node->cache + node->s + pos, (node->e - node->s) - s_pos);
	memcpy(new_node->cache + (node->e - node->s) - s_pos, data, size);
	new_node->s = 0;
	new_node->e = claim_size - 1;

	node->e = node->s+s_pos;
	if (node->e==node->s)
	{
		linked_list_del(&node->list);
	}

	caches->dat_size += size;

	return 0;
}

