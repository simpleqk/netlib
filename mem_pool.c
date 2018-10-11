#include "mem_pool.h"
#include "log.h"
#include "thread_lock.h"
#include "net_error.h"
#include <stdlib.h>


//align 4 algorithm
#define ALIGN4(n) ((n+3)&~3)
//check n whether pow(2, N)
#define IS_POW2(n) (!(n&(n-1)))
//mem manager max slot
#define MAX_SLOT_COUNT (15)
//max freed node count
#define MAX_FREE_NODE_COUNT (20)
//the number of mem-block created everytime
#define MEM_BLOCK_COUNT_PERCREATE (4)

//0, 1,2, 3, 4, 5,  6,  7,  8,   9,  10,  11,  12,  13,   14,    15
//                            1k   2k   4k   8k   16k   32k   64k
//0, 4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536
//total 15+1
#define MEM_POOL_SLOT_COUNT (16)

struct mem_block {
	unsigned int size;
	struct mem_block *next;
	char mem[0];
};

struct node {
        unsigned int use_count;
	unsigned int free_count;

	struct mem_block *freed_blk;
};

static struct node *g_slot_array[MEM_POOL_SLOT_COUNT];
static unsigned int g_limit_free_node_count = MAX_FREE_NODE_COUNT;

static struct tlock_t *g_tlock;
#define LOCK() lock_lock(g_tlock);
#define UNLOCK() lock_unlock(g_tlock);

static inline unsigned int mem_pool_near_pow2(unsigned int size)
{
	//register unsigned int v=1;
	register unsigned int s=size;

	//if (IS_POW2(s)) {
	//	return s;
	//}
	//
	//while(s) {
	//	v = v<<1;
	//	s = s>>1;
	//}
	//return v;
	
	s -= 1;
	s |= s>>1;
	s |= s>>2;
	s |= s>>4;
	s |= s>>8;
	s |= s>>16;
	return s+1;
}

static inline int mem_pool_hash(unsigned int size)
{
	//fast pow(x,2) 2<<(i-1)
	//0->0; 4->1; 8->2; 16->3 ...
	//fast log2
	//float-memory  1个sign标记正负+8个指数位+剩下23位数位
	float fval = (float)size;
	unsigned int *pfval = (unsigned int*)&fval;
	unsigned int ival = *pfval;
	unsigned int exp = (ival >> 23) & 0xff;
	return ((int)exp-127) -1;
}

static inline int mem_pool_choose_slot(unsigned int size)
{
	int slot = mem_pool_hash(size);

	if(slot>MAX_SLOT_COUNT) {
		slot = 0;
	}

	return slot;
}

static void mem_pool_destroy_mem_block(struct mem_block *blk)
{
	struct mem_block *last;
	while(blk) {
		last = blk;
		blk = blk->next;
		free(last);
	};
}

static struct mem_block* mem_pool_create_mem_block(unsigned int size, unsigned int count)
{
	struct mem_block *blk;
	struct mem_block *head, *last;
	unsigned int mbsize;

	head = NULL;
	last = NULL;
	count = (0==count) ? 1 : count;
	mbsize = sizeof(struct mem_block)+size;
	while(count--) {
		blk = (struct mem_block*)malloc(mbsize);
		if(blk) {
			if(NULL==head) {
				head = blk;
			}
			blk->size = size;
			blk->next = NULL;
			if(NULL==last) {
				last = blk;
			} else {
				last->next = blk;
				last = blk;
			}
		} else {
			mem_pool_destroy_mem_block(head);
			head = NULL;
		}
	}
	return head;
}

static struct node* mem_pool_create_node(unsigned int size, unsigned int count)
{
	struct node *nd = (struct node*)malloc(sizeof(struct node));
	if(nd) {
		nd->use_count = 0;
		nd->free_count = 0;
		nd->freed_blk = mem_pool_create_mem_block(size, count);
		if(nd->freed_blk) {
			nd->free_count = count;
		}
		else {
			free(nd);
			nd = NULL;
		}
	}

	return nd;
}


int mem_pool_init(unsigned int limit_free_node_count)
{
	if(limit_free_node_count >= 2) {
		g_limit_free_node_count = limit_free_node_count;
	}
	
	//thread lock
	if(NULL==g_tlock) {
		if(NULL == (g_tlock = lock_create_critical_section())) {
			return -1;
		}
	}

	return 0;
}

char* mem_pool_malloc(unsigned int size)
{
	long size_align;
	struct node **node_ref;
	struct node *node;
	struct mem_block *freeblock;
	char *mem;
	int slot, count;

	size = (0==size) ? 1 : size;
	size_align = ALIGN4(size);
	size_align = mem_pool_near_pow2(size_align);
	slot = mem_pool_choose_slot(size_align);
	count = (0==slot) ? 1 : MEM_BLOCK_COUNT_PERCREATE;

	//thread lock
	LOCK();
	node_ref = &g_slot_array[slot];
	node = *node_ref;
	if(NULL==node) {
		//node not exist
		node = mem_pool_create_node(size_align, count);
		if(node) {
			*node_ref = node;
		} else {
			LOG_WARN("[mem_pool] create_node failed.");
			UNLOCK();
			return NULL;
		}
	}
	
	if(NULL==node->freed_blk) {
		//no free mem_block
		node->freed_blk = mem_pool_create_mem_block(size_align, count);
		if(node->freed_blk) {
			node->free_count += count;
		} else {
			LOG_WARN("[mem_pool] create_mem_block failed.");
			UNLOCK();
			return NULL;
		}
	}

	//get mem block from freed_blk
	freeblock = node->freed_blk;
	node->freed_blk = node->freed_blk->next;
	freeblock->next = NULL;
	node->free_count--;
	node->use_count++;
	mem = freeblock->mem;
	UNLOCK();

	return mem;
}

int mem_pool_free(void *mem)
{
	struct node **node_ref, *nd;
	struct mem_block *blk;
	int slot = 0;

	if(NULL==mem) {
		LOG_WARN("[mem_pool] mem is invalid");
		return -1;
	}

	blk = (struct mem_block*)((char*)mem - sizeof(struct mem_block));
	if(!IS_POW2(blk->size)) {
		LOG_WARN("[mem_pool] mem_blk size is not pow2");
		return -1;
	}
	slot = mem_pool_choose_slot(blk->size);

	//thread lock
	LOCK();
	node_ref = &g_slot_array[slot];
	nd = *node_ref;

	if(0==slot || nd->free_count>=g_limit_free_node_count) {
		//large mem or free-count-limit, directly free
		free(blk);
		nd->use_count--;
		UNLOCK();
		return 0;
	}

	//put current mem to free_blk
	blk->next = nd->freed_blk;
	nd->freed_blk = blk;
	nd->use_count--;
	nd->free_count++;
	UNLOCK();

	return 0;
}

void mem_pool_release()
{
	struct node **nd=g_slot_array;
	int i=0;

	for(i=0;i<MEM_POOL_SLOT_COUNT;i++) {
		if(nd[i]) {
			mem_pool_destroy_mem_block(nd[i]->freed_blk);
			free(nd[i]);
			nd[i] = NULL;
		}
	}

	if(g_tlock) {
		lock_destroy(g_tlock);
		g_tlock = NULL;
	}
}

