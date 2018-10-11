/**********************************************************
* file: hash_map.h
* brief: hash map
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * brief: hash_map_func interface for custome to modify the
 *         default method
 * input: hash, cal hash function
 *        compare, compare between keys function
 *        assign_key, key assign between keys function
 *        assign_val, val assign between val function
 *        isvalid_key, check whether key is valid=1, invalid=0
 *        isvalid_val, check whether value is valid=1, invalid=0
 *        free_key, free assigned key resource
 *        free_val, free assigned value resource
 *
 *********************************************************/
struct hash_map_func {
	unsigned long (*hash)(long key);
	int (*compare)(long src_key, long dst_key);
	long (*assign_key)(long key);
	long (*assign_val)(long val);
	int (*isvalid_key)(long key);
	int (*isvalid_val)(long val);
	void (*free_key)(long key);
	void (*free_val)(long val);
};

enum EHMF_INNER {
	EFI_LONG_LONG = 0, //<long, long> func
	EFI_STR_STR,       //<string, string> func
	EFI_LONG_STR       //<long, string> func
};
/**********************************************************
 * brief: get pre-define hash_map_func
 * input: hmf, hash_map_func interface, return pre-deifne func
 *        type, inner pre-define hash_map_func
 *
 * return: 0 ok, -1 error
 *********************************************************/
int hash_map_inner_hmf(struct hash_map_func *hmf, enum EHMF_INNER type);

/**********************************************************
 * brief: create hash_map
 * input: size, hash_map size
 *        hmf, hash_map_func interface, stack memory also ok
 *
 * return: NULL error, other ok
 *********************************************************/
struct hash_map* hash_map_create(unsigned int size, struct hash_map_func *hmf);

/**********************************************************
 * brief: add key-value pair to hash_map
 * input: hmap, hash_map
 *        key, key value
 *        val, value
 *
 * return: 0 ok, -1 error
 *********************************************************/
int hash_map_add(struct hash_map *hmap, long key, long val);

/**********************************************************
 * brief: mod key-value pair to hash_map
 * input: hmap, hash_map
 *        key, key value
 *        val, value
 *
 * return: 0 ok, -1 error
 *********************************************************/
int hash_map_mod(struct hash_map *hmap, long key, long val);

/**********************************************************
 * brief: delete the key-value pair
 * input: hmap, hash_map
 *        key, key value
 *
 * return: 0 ok, -1 error
 *********************************************************/
int hash_map_del(struct hash_map *hmap, long key);

/**********************************************************
 * brief: find value from a pair of key-value
 * input: hmap, hash_map
 *        key, key value
 *        val, return value
 *
 * return: 0, find, -1 not find
 *********************************************************/
int hash_map_find(struct hash_map *hmap, long key, long *val);

/**********************************************************
 * brief: the count of key-value pair in hash_map
 * input: hmap, hash_map
 *
 * return: count
 *********************************************************/
unsigned int hash_map_count(struct hash_map *hmap);

/**********************************************************
 * brief: clear hash_map
 * input: hmap, hash map
 *
 * return: None
 *********************************************************/
void hash_map_clear(struct hash_map *hmap);

/**********************************************************
 * brief: destroy hash_map
 * input: hmap, hash map
 *
 * return: None
 *********************************************************/
void hash_map_destroy(struct hash_map *hmap);

#ifdef TEST_HASH_MAP_PERFORMANCE
/**********************************************************
 * brief: get hash_map performace info
 * input: hmap, hash map
 *        array, store performace data, the size is eq hash_map count
 *
 * return: 0 ok, -1 error
 *********************************************************/
int hash_map_show_performance(struct hash_map *hmap, long *array);
#endif //TEST_HASH_MAP_PERFORMANCE

#ifdef __cplusplus
}
#endif

#endif //_HASH_MAP_H_

