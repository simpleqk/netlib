#include "hash_map.h"
#include "mem_pool.h"
#include <string.h>
#include "log.h"
#include "net_error.h"

/**********************************************************
 * brief: hash_map for storing <long, long>
 *********************************************************/
inline static unsigned long (long_long_hash)(long key) {
	//hash*33 + c
	return ((key<<5) + key) + key;
}
inline static int (long_long_compare)(long src_key, long dst_key) {
	return (src_key>dst_key) ? (1) : ((src_key<dst_key) ? (-1) : (0));
}
inline static long (long_long_assign_key)(long key) {
	return key;
}
inline static long (long_long_assign_val)(long val) {
	return long_long_assign_key(val);
}
inline static int (long_long_isvalid_key)(long key) {
	//use variable only for compiler
	(void)key;
	return 1;
}
inline static int (long_long_isvalid_val)(long val) {
	//use variable only for compiler
	(void)val;
	return 1;
}
inline static void (long_long_free_key)(long key) {
	//use variable only for compiler
	(void)key;
}
inline static void (long_long_free_val)(long val) {
	//use variable only for compiler
	(void)val;
}


/**********************************************************
 * brief: hash_map for storing <string, string>
 *********************************************************/
inline static unsigned long (str_str_hash)(long key) {
	//hash*33 + c
	register const char *s = (const char*)key;
	register unsigned long hs=0;
	while(s) {
		hs = ((hs<<5) + hs) + (*s++);
	}
	return hs;
}
inline static int (str_str_compare)(long src_key, long dst_key) {
	register const char *s = (const char*)src_key;
	register const char *d = (const char*)dst_key;
	register int r = 0;
	while(1) {
		if(*s>*d) {r=1;break;}
		else if(*s<*d) {r=-1;break;};
		if('\0'==*s++ && '\0'==*d++) break;
	}
	return r;
}
inline static long (str_str_assign_key)(long key) {
	register const char *s = (const char*)key;
	register char *d = NULL;
	register char *ds = NULL;
	register unsigned int len = 0;
	
	while('\0' != s++) ++len;
	s = (const char*)key;
	d = mem_pool_malloc(len);
	if (d) {
		ds = d;
		while('\0'!=*s) *d++ = *s++;
		*d = '\0';
	}
	return (long)ds;
}
inline static long (str_str_assign_val)(long val) {
	return str_str_assign_key(val);
}
inline static int (str_str_isvalid_key)(long key) {
	return (0==key) ? (0) : (1);
}
inline static int (str_str_isvalid_val)(long val) {
	return (0==val) ? (0) : (1);
}
inline static void (str_str_free_key)(long key) {
	if(key) { mem_pool_free((char*)key); }
}
inline static void (str_str_free_val)(long val) {
	if(val) { mem_pool_free((char*)val); }
}

/**********************************************************
 * brief: hash_map for storing <long, string>
 *********************************************************/
inline static unsigned long (long_str_hash)(long key) {
	//hash*33 + c
	return ((key<<5) + key) + key;
}
inline static int (long_str_compare)(long src_key, long dst_key) {
	return (src_key>dst_key) ? (1) : ((src_key<dst_key) ? (-1) : (0));
}
inline static long (long_str_assign_key)(long key) {
	return key;
}
inline static long (long_str_assign_val)(long val) {
	return str_str_assign_val(val);
}
inline static int (long_str_isvalid_key)(long key) {
	//use variable only for compiler
	(void)key;
	return 1;
}
inline static int (long_str_isvalid_val)(long val) {
	return (0==val);
}
inline static void (long_str_free_key)(long key) {
	//use variable only for compiler
	(void)key;
}
inline static void (long_str_free_val)(long val) {
	if(val) { mem_pool_free((char*)val); }
}

/**********************************************************/
int hash_map_inner_hmf(struct hash_map_func *hmf, enum EHMF_INNER type)
{
	if(NULL==hmf) {
		return -1;
	}

	switch(type) {
		case EFI_LONG_LONG:
			hmf->hash = long_long_hash;
			hmf->compare = long_long_compare;
			hmf->assign_key = long_long_assign_key;
			hmf->assign_val = long_long_assign_val;
			hmf->isvalid_key = long_long_isvalid_key;
			hmf->isvalid_val = long_long_isvalid_val;
			hmf->free_key = long_long_free_key;
			hmf->free_val = long_long_free_val;
			return 0;
			break;
		case EFI_STR_STR:
			hmf->hash = str_str_hash;
			hmf->compare = str_str_compare;
			hmf->assign_key = str_str_assign_key;
			hmf->assign_val = str_str_assign_val;
			hmf->isvalid_key = str_str_isvalid_key;
			hmf->isvalid_val = str_str_isvalid_val;
			hmf->free_key = str_str_free_key;
			hmf->free_val = str_str_free_val;
			return 0;
			break;
		case EFI_LONG_STR:
			hmf->hash = long_str_hash;
			hmf->compare = long_str_compare;
			hmf->assign_key = long_str_assign_key;
			hmf->assign_val = long_str_assign_val;
			hmf->isvalid_key = long_str_isvalid_key;
			hmf->isvalid_val = long_str_isvalid_val;
			hmf->free_key = long_str_free_key;
			hmf->free_val = long_str_free_val;
			return 0;
			break;
		default:
			return -1;
			break;
	}
}

//=========================================================
struct hash_map {
	//interface
	struct hash_map_func hmf;

	unsigned int size;
	unsigned int count;

	struct node {
		struct node *next;
		long key;
		long val;
	}
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf {
		long hash;
		long nums;
		struct node *nd;
	}
#endif //TEST_HASH_MAP_PERFORMANCE
	*node_head[0];
};

inline static struct node* hash_map_create_node(struct hash_map_func *hmf, long key, long value)
{
	struct node *nd = (struct node*)mem_pool_malloc(sizeof(struct node));
	if(nd) {
		nd->next = NULL;
		nd->key = hmf->assign_key(key);
		nd->val = hmf->assign_val(value);
	}

	return nd;
}
inline static void hash_map_free_node(struct node* nd)
{
	mem_pool_free(nd);
}


struct hash_map* hash_map_create(unsigned int size, struct hash_map_func *hmf)
{
	long total_size = 0;
	struct hash_map *hm = NULL;

	if(0==size || NULL==hmf) {
		return NULL;
	}

#ifdef TEST_HASH_MAP_PERFORMANCE
	total_size = sizeof(struct hash_map) + sizeof(struct node_perf*)*size;
#else
	total_size = sizeof(struct hash_map) + sizeof(struct node*)*size;
#endif //TEST_HASH_MAP_PERFORMANCE

	////long total_size = (total_size+3) & ~3;
 	if((hm = (struct hash_map*)mem_pool_malloc(total_size))) {
		memset(hm, 0, total_size);
		hm->hmf = *hmf;
		hm->size = size;
	}

	return hm;
}
/*
int hash_map_set(struct hash_map *hmap, long key, long val)
{
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf **last_np;
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCE
	struct node **last_nd;
	struct node *nd;
	unsigned int hash;
	long v;

	if(NULL==hmap 
	  || !hmap->hmf.isvalid_key(key) || !hmap->hmf.isvalid_val(val)) {
		return -1;
	}

	hash = hmap->hmf.hash(key) % hmap->size;
#ifdef TEST_HASH_MAP_PERFORMANCE
	last_np = &hmap->node_head[hash];
	np = *last_np;
	last_nd = NULL;
	nd = NULL;
	if(np) {
		last_nd = &np->nd;
		nd = *last_nd;
	}
	else {
		//not exist, create node
		np = (struct node_perf*)mem_pool_malloc(sizeof(struct node_perf));
		if(np) {
			nd = hash_map_create_node(&hmap->hmf, key, val);
			if(nd) {
				np->nd = nd;
				np->hash = hash;
				np->nums = 1;
				*last_np = np;
				return 0;
			}
			else {
				hash_map_free_node(np);
				return -1;
			}
		}
		else {
			return -1;
		}
	}
#else
	last_nd = &hmap->node_head[hash];
	nd = *last_nd;
#endif //TEST_HASH_MAP_PERFORMANCE
	if(nd) {
		do {
			//check wheter exist
			if(0==hmap->hmf.compare(nd->key, key)) {
				v = hmap->hmf.assign_val(val);
				if(hmap->hmf.isvalid_val(v)) {
					hmap->hmf.free_val(nd->val);
					nd->val = v;
					hmap->count++;
#if TEST_HASH_MAP_PERFORMANCE
					np->nums += 1;
#endif //TEST_HASH_MAP_PERFORMANCE
					return 0;
				}
				return -1;
			}

			last_nd = &nd->next;
			nd = nd->next;
		}while(nd);
	}

	//not exist, create new node
	nd = hash_map_create_node(&hmap->hmf, key, val);
	if(nd) {
		*last_nd = nd;
		hmap->count++;
#if TEST_HASH_MAP_PERFORMANCE
		np->nums += 1;
#endif //TEST_HASH_MAP_PERFORMANCE
		return 0;
	}
	else {
		return -1;
	}
}*/

int hash_map_add(struct hash_map *hmap, long key, long val)
{
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf **last_np;
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCE
	struct node **last_nd;
	struct node *nd;
	unsigned int hash;

	if(NULL==hmap 
	  || !hmap->hmf.isvalid_key(key) || !hmap->hmf.isvalid_val(val)) {
		return -1;
	}

	hash = hmap->hmf.hash(key) % hmap->size;
#ifdef TEST_HASH_MAP_PERFORMANCE
	last_np = &hmap->node_head[hash];
	np = *last_np;
	last_nd = NULL;
	nd = NULL;
	if(np) {
		last_nd = &np->nd;
		nd = *last_nd;
	}
	else {
		//not exist, create node
		np = (struct node_perf*)mem_pool_malloc(sizeof(struct node_perf));
		if(np) {
			nd = hash_map_create_node(&hmap->hmf, key, val);
			if(nd) {
				np->nd = nd;
				np->hash = hash;
				np->nums = 1;
				*last_np = np;
				return 0;
			}
			else {
				mem_pool_free(np);
				return -1;
			}
		}
		else {
			return -1;
		}
	}
#else
	last_nd = &hmap->node_head[hash];
	nd = *last_nd;
#endif //TEST_HASH_MAP_PERFORMANCE
	if(nd) {
		do {
			//check wheter exist
			if(0==hmap->hmf.compare(nd->key, key)) {
				return -1;
			}

			last_nd = &nd->next;
			nd = nd->next;
		}while(nd);
	}

	//not exist, create new node
	nd = hash_map_create_node(&hmap->hmf, key, val);
	if(nd) {
		*last_nd = nd;
		hmap->count++;
#if TEST_HASH_MAP_PERFORMANCE
		np->nums += 1;
#endif //TEST_HASH_MAP_PERFORMANCE
		return 0;
	}
	else {
		return -1;
	}
}

int hash_map_mod(struct hash_map *hmap, long key, long val)
{
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf **last_np;
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCE
	struct node **last_nd;
	struct node *nd;
	unsigned int hash;
	long v;

	if(NULL==hmap 
	  || !hmap->hmf.isvalid_key(key) || !hmap->hmf.isvalid_val(val)) {
		return -1;
	}

	hash = hmap->hmf.hash(key) % hmap->size;
#ifdef TEST_HASH_MAP_PERFORMANCE
	last_np = &hmap->node_head[hash];
	np = *last_np;
	last_nd = NULL;
	nd = NULL;
	if(np) {
		last_nd = &np->nd;
		nd = *last_nd;
	}
	else {
		//not exist
		return -1;
	}
#else
	last_nd = &hmap->node_head[hash];
	nd = *last_nd;
#endif //TEST_HASH_MAP_PERFORMANCE
	if(nd) {
		do {
			//check wheter exist
			if(0==hmap->hmf.compare(nd->key, key)) {
				v = hmap->hmf.assign_val(val);
				if(hmap->hmf.isvalid_val(v)) {
					hmap->hmf.free_val(nd->val);
					nd->val = v;
					hmap->count++;
#if TEST_HASH_MAP_PERFORMANCE
					np->nums += 1;
#endif //TEST_HASH_MAP_PERFORMANCE
					return 0;
				}
				return -1;
			}

			last_nd = &nd->next;
			nd = nd->next;
		}while(nd);
	}

	//not exist
	return -1;
}

int hash_map_del(struct hash_map *hmap, long key)
{
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf **last_np;
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCE
	struct node **last_nd;
	struct node *nd;
	unsigned long hash;
	long first;

	if(NULL==hmap || !hmap->hmf.isvalid_key(key)) {
		return -1;
	}

	hash = hmap->hmf.hash(key) % hmap->size;
#ifdef TEST_HASH_MAP_PERFORMANCE
	last_np = &hmap->node_head[hash];
	np = *last_np;
	last_nd = NULL;
	nd = NULL;
	if(np) {
		last_nd = &np->nd;
		nd = *last_nd;
	}
#else
	last_nd = &hmap->node_head[hash];
	nd = *last_nd;
#endif //TEST_HASH_MAP_PERFORMANCE
	first=1;
	if(nd) {
		do {
			//check wheter exist
			if(0==hmap->hmf.compare(nd->key, key)) {
				//free key, val and node
				hmap->hmf.free_key(nd->key);
				hmap->hmf.free_val(nd->val);
				if(first) {
					*last_nd = nd->next;
				}
				else {
					(*last_nd)->next = nd->next;
				}
				hash_map_free_node(nd);
#ifdef TEST_HASH_MAP_PERFORMANCE
				np->nums -= 1;
#endif //TEST_HASH_MAP_PERFORMANCE
				return 0;
			}
			last_nd = &nd;
			nd = nd->next;
			first = 0;
		}while(nd);
	}

	//not exist
	return -1;
}

int hash_map_find(struct hash_map *hmap, long key, long *val)
{
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCE
	unsigned long hash;
	struct node *nd;

	if(NULL==hmap || !hmap->hmf.isvalid_val(key) || NULL==val) {
		return -1;
	}

	hash = hmap->hmf.hash(key) % hmap->size;
#ifdef TEST_HASH_MAP_PERFORMANCE
	nd = NULL;
	np = hmap->node_head[hash];
	if(np) {
		nd = np->nd;
	}
#else
	nd = hmap->node_head[hash];
#endif //TEST_HASH_MAP_PERFORMANCE
	if(nd) {
		do {
			//check wheter exist
			if(0==hmap->hmf.compare(nd->key, key)) {
				*val = nd->val;
				return 0;
			}
			nd = nd->next;
		}while(nd);
	}

	return -1;
}

unsigned int hash_map_count(struct hash_map *hmap)
{
	return (hmap) ? (hmap->count) : (0);
}

void hash_map_clear(struct hash_map *hmap)
{
	unsigned int i;
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCEunsigned int i;
	struct node *nd, *prev;
	if(hmap) {
		for(i=0;i<hmap->count;i++) {
#ifdef TEST_HASH_MAP_PERFORMANCE
			if((np = hmap->node_head[i])) {
				nd = np->nd;
				hash_map_free_node(np);
			} else {
				continue;
			}
#else
			nd = hmap->node_head[i];
#endif //TEST_HASH_MAP_PERFORMANCE
			if(nd) {
				do {//free list
					hmap->hmf.free_key(nd->key);
					hmap->hmf.free_val(nd->val);
					prev = nd;
					nd = nd->next;
					hash_map_free_node(prev);
				}while(nd);
			}
			hmap->node_head[i] = NULL;
		}
		hmap->count = 0;
	}
}

void hash_map_destroy(struct hash_map *hmap)
{
	unsigned int i;
#ifdef TEST_HASH_MAP_PERFORMANCE
	struct node_perf *np;
#endif //TEST_HASH_MAP_PERFORMANCEunsigned int i;
	struct node *nd, *prev;
	if(hmap) {
		for(i=0;i<hmap->count;i++) {
#ifdef TEST_HASH_MAP_PERFORMANCE
			if((np = hmap->node_head[i])) {
				nd = np->nd;
				hash_map_free_node(np);
			} else {
				continue;
			}
#else
			nd = hmap->node_head[i];
#endif //TEST_HASH_MAP_PERFORMANCE
			if(nd) {
				do {//free list
					hmap->hmf.free_key(nd->key);
					hmap->hmf.free_val(nd->val);
					prev = nd;
					nd = nd->next;
					hash_map_free_node(prev);
				}while(nd);
			}
		}
		mem_pool_free(hmap);
	}
}

#ifdef TEST_HASH_MAP_PERFORMANCE
int hash_map_show_performance(struct hash_map *hmap, long *array)
{
	unsigned int i;
	struct node_head *nd;
	
	if(NULL==hmap || NULL==array) {
		return -1;
	}

	if(hmap) {
		for(i=0;i<hmap->count;i++) {
			if((nd = hmap->node_head[i])) {
				array[i] = nd->nums;
				//printf("hash_map bucket[%07d]: count=%07ld, hash=%ld\n", i, nd->nums, nd->hash);
			}
			else {
				array[i] = 0;
				//printf("hash_map bucket[%07d]: null\n", i);
			}
		}
	}
}
#endif //TEST_HASH_MAP_PERFORMANCE

