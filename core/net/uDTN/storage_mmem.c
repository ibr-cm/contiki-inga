/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup bundle_storage_mmem MMEM-based temporary Storage
 *
 * @{
 */

/**
 * \file 
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "lib/mmem.h"
#include "lib/list.h"
#include "logging.h"

#include "bundle.h"
#include "sdnv.h"
#include "agent.h"
#include "statusreport.h"
#include "profiling.h"
#include "statistics.h"

#include "storage.h"

//FIXME dummy index block
static struct bundle_index_entry_t temp_index_array[BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS] = { 0 };
static uint8_t temp_index_array_toggle = 0;
static uint16_t temp_index_array_collision_check[BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS] = { 0 };

//      * index_newest_block
//      storage_cached_get_index_block()
//static uint8_t last_index_entry = BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS+1;
/** Last written block with index data, initialized with invalid value */  //FIXME this replaces temp_index_array
//static uint8_t last_index_block_address = CACHE_PARTITION_B_INDEX_INVALID_TAG;

// defined in mmem.c, no function to access it though
extern unsigned int avail_memory;

/**
 * Internal representation of a bundle
 *
 * The layout is quite fixed - the next pointer and the bundle_num have to go first because this struct
 * has to be compatible with the struct storage_entry_t in storage.h!
 */
struct bundle_list_entry_t {  //durch bundle_index_entry_t ersetzen
	/** pointer to the next list element */
	struct bundle_list_entry_t * next;

	/** copy of the bundle number - necessary to have
	 * a static address that we can pass on as an
	 * argument to an event
	 */
	uint32_t bundle_num;

	/** pointer to the actual bundle stored in MMEM */
	struct mmem *bundle;
};

// List and memory blocks for the bundles
LIST(bundle_list);
MEMB(bundle_mem, struct bundle_list_entry_t, BUNDLE_STORAGE_SIZE);

// global, internal variables
/** Counts the number of bundles in storage */
static uint16_t bundles_in_storage;

/** Is used to periodically traverse all bundles and delete those that are expired */
static struct ctimer r_store_timer;

/**
 * "Internal" functions
 */
void storage_mmem_prune();
uint8_t storage_mmem_flush(void);
uint8_t storage_mmem_delete_bundle_by_bundle_number(uint32_t bundle_number);
void storage_mmem_update_statistics();

/**
 * \brief internal function to send statistics to statistics module
 */
void storage_mmem_update_statistics() {
	statistics_storage_bundles(bundles_in_storage);
	statistics_storage_memory(avail_memory);
}

/**
 * \brief called by agent at startup
 */
uint8_t storage_mmem_init(void)
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "storage_mmem init");

	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	// Initialize MMEM for the binary bundle storage
	mmem_init();

	bundles_in_storage = 0;

	storage_mmem_flush();  //FIXME das sollte hier später raus. clean != leer...
	storage_mmem_update_statistics();

	ctimer_set(&r_store_timer, CLOCK_SECOND*5, storage_mmem_prune, NULL);

	return 1;
}

/**
 * \brief deletes expired bundles from storage
 */
void storage_mmem_prune()
{
	uint32_t elapsed_time;
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
		elapsed_time = clock_seconds() - bundle->rec_time;

		if( bundle->lifetime < elapsed_time ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "bundle lifetime expired of bundle %lu", entry->bundle_num);
            printf("PRUNE: bundle lifetime expired of bundle %lu\n", entry->bundle_num); //FIXME
			storage_mmem_delete_bundle_by_bundle_number(bundle->bundle_num);
		}
	}

	ctimer_restart(&r_store_timer);
}

/**
 * \brief deletes all stored bundles + index blocks
 */
uint8_t storage_mmem_flush(void)
{
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Delete all bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		storage_mmem_delete_bundle_by_bundle_number(bundle->bundle_num);
	}
	//FIXME delete index blocks

	return 1;
}

/**
 * \brief This function delete as many bundles from the storage as necessary to have at least one slot and the number of required of memory free
 * \param bundlemem Pointer to the MMEM struct containing the bundle
 * \return 1 on success, 0 if no room could be made free
 */
uint8_t storage_mmem_make_room(struct mmem * bundlemem)
{
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t * bundle_new = NULL;
	struct bundle_t * bundle_old = NULL;

	/* Now delete expired bundles */  //FIXME not what we want...
	storage_mmem_prune();

	/* If we do not have a pointer, we cannot compare - do nothing */
	if( bundlemem == NULL ) {
		return 0;
	}

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		/* Obtain the new pointer each time, since the address may change */
		bundle_new = (struct bundle_t *) MMEM_PTR(bundlemem);

		/* We need this double-loop because otherwise we would be modifying the list
		 * while iterating through it
		 */
		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {
			bundle_old = (struct bundle_t *) MMEM_PTR(entry->bundle);

			/* If the new bundle has a longer lifetime than the bundle in our storage,
			 * delete the bundle from storage to make room
			 */
			if( bundle_new->lifetime - (clock_seconds() - bundle_new->rec_time) >= bundle_old->lifetime - (clock_seconds() - bundle_old->rec_time) ) {
				break;
			}
		}

		/* Either the for loop did nothing or did not break */
		if( entry == NULL ) {
			/* We do not have deletable bundles in storage, stop deleting them */
			return 0;
		}

		/* Delete Bundle */
		storage_mmem_delete_bundle_by_bundle_number(entry->bundle_num);
	}

	return 1;
}

/**
 * \brief saves a bundle in storage
 * \param bundlemem pointer to the bundle
 * \param bundle_number_ptr pointer where the bundle number will be stored (on success)
 * \return 0 on error, 1 on success
 */
uint8_t storage_mmem_save_bundle(struct mmem * bundlemem, uint8_t flags)
{
	struct bundle_t *entrybdl = NULL,
					*bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "storage_mmem_save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "storage_mmem_save_bundle with invalid MMEM structure");
		return 0;
	}

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		entrybdl = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle->bundle_num == entrybdl->bundle_num ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "%lu is the same bundle", entry->bundle_num);
			bundle_decrement(bundlemem);
			return 1;
		}
	}

	if( !storage_mmem_make_room(bundlemem) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot store bundle, no room");
		return 0;
	}

	// Now we have to update the pointer to our bundle, because MMEM may have been modified (freed) and thus the pointer may have changed
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot store bundle");
		bundle_decrement(bundlemem);
		return 0;
	}

	// Clear the memory area
	memset(entry, 0, sizeof(struct bundle_list_entry_t));

	// we copy the reference to the bundle, therefore we have to increase the reference counter
	entry->bundle = bundlemem;
	bundle_increment(bundlemem);
	bundles_in_storage++;

	// Set bundle number //FIXME deprecated
	entry->bundle_num = bundle->bundle_num;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu (%lu), Src %lu, Dest %lu, Seq %lu", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

	// Notify the statistics module
	storage_mmem_update_statistics();

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Now we have to (virtually) free the incoming bundle slot
	// This should do nothing, as we have incremented the reference counter before
	bundle_decrement(bundlemem);

    printf("save_done: RT: %lu , NB: %u , SN: %lu , SS: %lu , DN: %lu , DS: %lu , SeqNr: %lu , LT: %lu, ID: %lu \n",
            bundle->rec_time, bundle->num_blocks, bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, bundle->tstamp_seq, bundle->lifetime, bundle->bundle_num); //FIXME
	//FIXME für letztes storage segment, agent mitteilen, dass wir alles haben
    if( flags == STORAGE_NO_SEGMENT || flags == STORAGE_LAST_SEGMENT ) {
        process_post(&agent_process, dtn_bundle_in_storage_event, &bundle->bundle_num);
    }

	return 1;
}

uint8_t storage_mmem_delete_bundle_by_index_entry(struct bundle_index_entry_t *index_entry){
    storage_mmem_delete_bundle_by_bundle_number(index_entry->bundle_num); //FIXME oder so
    //FIXME indexeintrag löschen
    return 1;
}
/**
 * \brief deletes a bundle from storage
 * \param bundle_number bundle number to be deleted
 * \param reason reason code
 * \return 1 on success or 0 on error
 */
uint8_t storage_mmem_delete_bundle_by_bundle_number(uint32_t bundle_number)
{
	struct bundle_t * bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Deleting Bundle %lu", bundle_number);
    printf("Deleting Bundle %lu\n", bundle_number); //FIXME

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Could not find bundle %lu on storage_mmem_delete_bundle", bundle_number);
		return 0;
	}

	//FIXME das machen wir woanders...
	// Figure out the source to send status report
//	bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
//	bundle->del_reason = reason;
//
//	if( reason != REASON_DELIVERED ) {
//		if( (bundle->flags & BUNDLE_FLAG_CUST_REQ ) || (bundle->flags & BUNDLE_FLAG_REP_DELETE) ){
//			if (bundle->src_node != dtn_node_id){
//				STATUSREPORT.send(entry->bundle, 16, bundle->del_reason);
//			}
//		}
//	}

	//FIXME das auch
	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	//FIXME slot trotzdem belegen, evtl. "valid"-flag
	bundle_decrement(entry->bundle);
	bundle = NULL;

	//FIXME naja
	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	bundles_in_storage--;

	// Notify the statistics module
	storage_mmem_update_statistics();

	//FIXME mal schaun
	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
 * \brief reads a bundle from storage
 * \param bundle_num bundle number to read
 * \return pointer to the MMEM struct, NULL on error
 */
struct mmem *storage_mmem_read_bundle(uint32_t bundle_num, uint32_t block_data_start_offset, uint16_t block_data_length)
{
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t * bundle = NULL;

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle->bundle_num == bundle_num ) {
			break;
		}
	}
    printf("read_done: RT: %lu , NB: %u , SN: %lu , SS: %lu , DN: %lu , DS: %lu , SeqNr: %lu , LT: %lu, ID: %lu \n",
            bundle->rec_time, bundle->num_blocks, bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, bundle->tstamp_seq, bundle->lifetime, bundle->bundle_num); //FIXME

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Could not find bundle %lu in storage_mmem_read_bundle", bundle_num);
		return 0;
	}

	if( entry->bundle->size == 0 ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Found bundle %lu but file size is %u", bundle_num, entry->bundle->size);
		return 0;
	}

	// Someone requested the bundle, he will have to decrease the reference counter again
	bundle_increment(entry->bundle);

	/* How long did this bundle rot in our storage? */
	uint32_t elapsed_time = clock_seconds() - bundle->rec_time;

	/* Update lifetime of bundle */
	if( bundle->lifetime < elapsed_time ) {
		bundle->lifetime = 0;
		bundle->rec_time = clock_seconds();
	} else {
		bundle->lifetime = bundle->lifetime - elapsed_time;
		bundle->rec_time = clock_seconds();
	}

	return entry->bundle;
}

/**
 * \brief checks if there is space for a bundle
 * \param bundlemem pointer to a bundle struct (not used here)
 * \return number of free slots
 */
uint32_t storage_mmem_get_free_space()
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
 * \brief Get the number of slots available in storage
 * \returns the number of free slots
 */
uint16_t storage_mmem_get_bundle_count(void){
	return bundles_in_storage;
}

/**
 * \brief Get the bundle list
 * \returns pointer to first bundle list entry
 */
struct mmem *storage_mmem_get_index_block(uint8_t blocknr){

    int ret;
    struct bundle_slot_t *bs;
    struct bundle_t *index_block;

    bs = bundleslot_get_free();

    if( bs == NULL ) {
        LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate slot for a bundle");
        return NULL;
    }

    //FIXME
    //ret = mmem_alloc(&bs->bundle, sizeof(struct bundle_t));
    ret = mmem_alloc(&bs->bundle, MIN_BUNDLESLOT_SIZE);
    if (!ret) {
        bundleslot_free(bs);
        LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate memory for a bundle");
        return NULL;
    }

    index_block = (struct bundle_t *) MMEM_PTR(&bs->bundle);
    //memset(index_block, 0, sizeof(struct bundle_t));
    memset(index_block, 0, MIN_BUNDLESLOT_SIZE);

    //FIXME simuliert Ende der Liste
    if(temp_index_array_toggle == 1){
        temp_index_array_toggle = 0;
        return NULL;
    } else {
        temp_index_array_toggle = 1;
    }

    //get "next" index block from cache
    //struct cache_entry_t cache_block = BUNDLE_CACHE.cache_access_partition(CACHE_PARTITION_NEXT_BLOCK, CACHE_PARTITION_B_INDEX_START, last_index_block_address);

//    uint8_t i;
//    for(i=0; i<BUNDLE_STORAGE_INDEX_ENTRYS; ++i){
//      temp_index_array[i].bundle_num=i+1; //FIXME ID
//      temp_index_array[i].dst_node=i+1;  //FIXME Zielnode
//    }
//
//    //FIXME nur bei RAM-Block nötig
//    for(i=0; i<BUNDLE_STORAGE_INDEX_ENTRYS; ++i){
//          if(temp_index_array[i].bundle_num == 0 && temp_index_array[i].dst_node == 0){
//              *index_array_entrys = i-1;
//              break;
//          }
//    }
//
//
// TEST
//  int main(void){
//          struct storage_index_entry_t *array_ptr = NULL;
//          int array_length = 0;
//          array_ptr = storage_cached_get_bundles(STORAGE_CACHED_GET_BUNDLES_HEAD, &array_length);
//
//          int i;
//          for(i=0; i<array_length; ++i){
//                  printf("Bundle[%d] : [ID] %d : [Dest] %d\n",i,array_ptr[i].bundle_num,array_ptr[i].dst_node);
//          }
//          return 0;
//  }

    //return temp_index_array;

    //FIXME Beim 2. - n. Aufruf: Array 2 - n
    //      Dann 1mal länge 0
    //      Dann wieder von vorne
    //      Blockiert bis zum nächsten read den entsprechenden Cacheblock, d.h. Liste muss komplett durchlaufen werden
    //      state pro aufrufer?
    //      freigeben von liste?
    //      Kümmert sich um das Nachladen vom Flash

    return &bs->bundle;
}

uint8_t storage_mmem_housekeeping(uint16_t time){
    return 1;
}
uint8_t storage_mmem_release_bundleslots(uint16_t size){
    return 0;
}
uint8_t storage_mmem_add_segment_to_bundle(struct mmem *bundlemem, uint16_t min_size){
    return 0;
}

const struct storage_driver storage_mmem = {
	"STORAGE_MMEM",
	storage_mmem_init,
	storage_mmem_flush,
    storage_mmem_housekeeping,
    storage_mmem_release_bundleslots,
	storage_mmem_save_bundle,
	storage_mmem_add_segment_to_bundle,
    storage_mmem_delete_bundle_by_bundle_number,
	storage_mmem_delete_bundle_by_index_entry,
    storage_mmem_read_bundle,
	storage_mmem_get_free_space,
	storage_mmem_get_bundle_count,
	storage_mmem_get_index_block,
};

/** @} */
/** @} */
