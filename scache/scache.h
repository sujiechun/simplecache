#ifndef _SCACHE_H_
#define _SCACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct _scaches_config_
{
	int max_size;
	int min_rd;	//��С������
	int max_rd;	//�������
	float ard;	//�������
};

typedef struct _scaches_ scache;
typedef struct _scaches_config_ scache_config;
typedef struct _recycled_caches_ recycle_pool;

extern void scache_module_init();
extern scache* new_scache(scache_config* config, recycle_pool* pool);
extern char* caches_read(struct _scaches_* caches, int pos, int size);
extern int caches_delete(struct _scaches_* caches, int pos, int size);
extern int caches_insert(struct _scaches_* caches, int pos, char* data, int size);

#ifdef __cplusplus
}
#endif
#endif