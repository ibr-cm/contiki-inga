/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup bundle_storage_cached RAM cached persistent Storage
 *
 * @{
 */

/**
 * \file 
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "lib/list.h"
#include "logging.h"

#include "profiling.h"

#include "cache.h"

void memb_cache_init(){
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "memb_cache init");
}

struct cache_entry_t * memb_cache_access_block(uint16_t persistent_address){
	struct cache_entry_t entry;
	return entry;
}

uint8_t memb_cache_write_back(uint16_t persistent_address){
	return 0;
}

struct cache_entry_t * memb_cache_access_partition(uint8_t mode, uint16_t partition_start, uint16_t partition_end){

	static uint8_t delivered_index_blocks[CACHE_BLOCKS_NUM] = { 0 };
	static uint8_t last_delivered_address = CACHE_PARTITION_B_INDEX_INVALID_TAG;

	if (partition_end == CACHE_PARTITION_B_INDEX_INVALID_TAG){
		//nur RAM-Block
	}

	struct cache_entry_t entry;
	return entry;
}

const struct cache_driver memb_cache = {
	"MEMB_CACHE",
	memb_cache_init,
	memb_cache_access_block,
	memb_cache_write_back,
	memb_cache_access_partition,
};

/** @} */
/** @} */
