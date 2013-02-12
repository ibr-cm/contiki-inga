/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle_storage_cache MEMB-based cache
 *
 * @{
 */

/**
 * \file 
 * \brief this file defines the interface for the bundle cache
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "memb.h"

/**
 * Of how many blocks consists the cache?
 * Default is 10
 */
#ifdef CACHE_CONF_BLOCKS_NUM
#define CACHE_BLOCKS_NUM CACHE_CONF_BLOCKS_NUM
#else
#define CACHE_BLOCKS_NUM 10
#endif

/**
 * Size of one cache block?
 * Default is 528 (FLASH_PAGE_SIZE on INGA) //FIXME 512, var
 */
#ifdef CACHE_CONF_BLOCKS_SIZE
#define CACHE_BLOCKS_SIZE CACHE_CONF_BLOCKS_SIZE
#else
#define CACHE_BLOCKS_SIZE 528
#endif

/**
 * Define partitions for quotas.
 *
 */
#define CACHE_PARTITION_B_INDEX 0
#define CACHE_PARTITION_BUNDLES 1

#define CACHE_PARTITION_B_INDEX_START 0
#define CACHE_PARTITION_B_INDEX_END 60
#define CACHE_PARTITION_B_INDEX_INVALID_TAG 4096

#define CACHE_PARTITION_BUNDLES_START 61
#define CACHE_PARTITION_BUNDLES_END 4095
#define CACHE_PARTITION_BUNDLES_INVALID_TAG 4097

#define CACHE_PARTITION_B_INDEX_MIN 1
#define CACHE_PARTITION_B_INDEX_MAX 10 //FIXME erscheint redundant...

#define CACHE_PARTITION_BUNDLES_MIN 0
#define CACHE_PARTITION_BUNDLES_MAX 9

/* */
#define CACHE_PARTITION_RESET 0
#define CACHE_PARTITION_NEXT_BLOCK 1

/**
 * Representation of a cache entry
 */
struct cache_entry_t {
	/** pointer to the next list element */
	struct cache_entry_t * next;

	/** Cache flags */
	uint16_t cache_flags; //FIXME Dirty, Use, Tag
	                      // 14 Bit Tag = 16384 data blocks are addressable

	/** Data Block*/
	//DATA_BLOCK FIXME
};

/** cache interface  */
struct cache_driver {
	char *name;
	/** called by agent a startup */
	void (* init)(void);
	/** returns cached data block of persistent_address or NULL if no cache block is available*/
	struct cache_entry_t *(* cache_access_block)(uint16_t persistent_address);
	/** writes cached data block tagged with persistent_address to persistent storage (if necessary)*/
	uint8_t (* cache_write_back)(uint16_t persistent_address);
	/** returns pointer to element of partition, keeps state of delivered elements, mode: next, reset */
	struct cache_entry_t * (* cache_access_partition)(uint8_t mode, uint16_t partition_start, uint16_t partition_end);
};
extern const struct cache_driver BUNDLE_CACHE;
#endif
/** @} */
/** @} */
