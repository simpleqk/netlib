/**********************************************************
* file: mem_pool.h
* brief: mem manager
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * brief: init mem pool
 * input: limit_free_node_count, the max count ot free node
 *
 * return: 0 ok, -1 error
 *********************************************************/
int mem_pool_init(unsigned int limit_free_node_count/*0/10*/);

/**********************************************************
 * brief: malloc mem block
 * input: size, mem size for malloc
 *
 * return: NULL error, other ok
 *********************************************************/
char* mem_pool_malloc(unsigned int size);

/**********************************************************
 * brief: free mem block
 * input: mem, mem block
 *
 * return: 0 ok, -1 error
 *********************************************************/
int mem_pool_free(void *mem);

/**********************************************************
 * brief: release mem pool
 * input: None
 *
 * return: None
 *********************************************************/
void mem_pool_release();

#ifdef __cplusplus
}
#endif

#endif //_MEM_POOL_H_

