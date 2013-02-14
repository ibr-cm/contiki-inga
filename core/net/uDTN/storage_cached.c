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
#include <string.h>

#include "contiki.h"
#include "lib/list.h"
#include "logging.h"

#include "bundle.h"
#include "agent.h"
#include "statusreport.h"
#include "profiling.h"
#include "statistics.h"
#include "hash.h"

#include "storage.h"
#include "cache.h"

#define STORAGE_CACHED_GET_BUNDLES_HEAD 0
#define STORAGE_CACHED_GET_BUNDLES_NEXT 1
#define STORAGE_CACHED_GET_BUNDLES_FREE 2

static uint8_t next_mode = STORAGE_CACHED_GET_BUNDLES_HEAD; //FIXME flags zusammenlegen

//FIXME dummy index block
//FIXME besser mehr dimensionen?
static struct storage_index_entry_t temp_index_array[BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS] = { 0 };
static uint8_t temp_index_array_toggle = 0;
static uint16_t temp_index_array_collision_check[BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS] = { 0 };

//      * index_newest_block
//      storage_cached_get_index_block()
static uint8_t last_index_entry = BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS+1;
/** Last written block with index data, initialized with invalid value */  //FIXME this replaces temp_index_array
//static uint8_t last_index_block_address = CACHE_PARTITION_B_INDEX_INVALID_TAG;

/**
 * Internal representation of a bundle
 *
 * The layout is quite fixed - the next pointer and the bundle_num have to go first because this struct
 * has to be compatible with the struct storage_entry_t in storage.h!
 */
struct bundle_list_entry_t {
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
void storage_cached_prune();
void storage_cached_reinit(void);
uint16_t storage_cached_delete_bundle(uint32_t bundle_number, uint8_t reason);
void storage_cached_update_statistics();

/**
 * \brief internal function to send statistics to statistics module
 */
void storage_cached_update_statistics() {
	statistics_storage_bundles(bundles_in_storage);
	statistics_storage_memory(/*FIXME*/);
}

/**
 * \brief called by agent at startup
 */
void storage_cached_init(void)
{
	//FIXME mal sehn
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "storage_cached init");

	//FIXME Cacheblock für Index reservieren

	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	bundles_in_storage = 0;

	storage_cached_reinit();
	storage_cached_update_statistics();

	ctimer_set(&r_store_timer, CLOCK_SECOND*5, storage_cached_prune, NULL);
}

/**
 * \brief adds index entry
 */
uint8_t storage_cached_add_index_entry(uint32_t ID, uint32_t TargetNode){
	//FIXME in dem Moment, in dem die gültige Adresse feststeht
	uint8_t i;
	for(i=last_index_entry+1; i<BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS; ++i){
		if(i>BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS){
			i=0;
		}
		if(temp_index_array[i].bundle_num == 0 && temp_index_array[i].dst_node == 0){
			temp_index_array[i].bundle_num = ID;
			temp_index_array[i].dst_node = TargetNode;
			last_index_entry = i;
			return 1;
		}
	}
	return 0;
}

/**
 * \brief finds index entry for ID, overwrites it with last_index_entry
 */
uint8_t storage_cached_del_index_entry(uint32_t ID){
    for(i=0; i<BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS; ++i){
        if(temp_index_array[i].bundle_num == ID){
            temp_index_array[i].bundle_num = temp_index_array[last_index_entry].bundle_num;
            temp_index_array[i].dst_node = temp_index_array[last_index_entry].dst_node;
            temp_index_array[last_index_entry].bundle_num = 0;
            temp_index_array[last_index_entry].dst_node = 0;
            --last_index_entry;
            return 1;
        }
    }
    return 0;

}

void storage_cached_build_index(){
	//FIXME scan through bundle storage
}

/**
 * \brief deletes expired bundles from storage
 */
void storage_cached_prune()
{
	//FIXME überprüft X Flashseiten auf abgelaufene Bündel
	//FIXME option "build index"
}

/**
 * \brief housekeeping for bundle storage
 */
void storage_cached_garbage_collect()
{
	//FIXME arbeitet Liste mit zu löschenden Bündeln ab
	//      dann storage_cached_prune()
}

/**
 * \brief calculates address on persistent storage
 */
void storage_cached_calculate_persistent_address()
{
	//FIXME "berechnet" Speicheradresse für (1.) Bündelblock
}

/**
 * \brief Sets the storage to its initial state
 */
void storage_cached_reinit(void)
{
	// Delete all bundles from storage
	//FIXME mal sehn
}

/**
 * \brief This function delete as many bundles from the storage as necessary to have at least one slot and the number of required of memory free
 * \param bundlemem Pointer to the MMEM struct containing the bundle
 * \return 1 on success, 0 if no room could be made free
 */
uint8_t storage_cached_make_room(struct mmem * bundlemem)
{
	return 1;
	//FIXME mal sehn
}

/**
 * \brief saves a bundle in storage
 * \param bundlemem pointer to the bundle
 * \param bundle_number_ptr pointer where the bundle number will be stored (on success)
 * \return 0 on error, 1 on success
 */
uint8_t storage_cached_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr){
	//FIXME if (cache_access(Ungültiges Bundle Tag)) =! NULL
	//        Cacheblock + Offset(s) merken
	//        auf weitere Segmente warten
	//        Anhängen
	//      else  //nur noch 1 Cacheblock nicht ungültig markiert, Adresse muss berechnet werden
	//        berechnet Speicheradresse (Fkt)
	//        cache_access(Gültiges Bundle Tag)
	//      oder
	//        vollen Block aus Cache mit gültigem Tag versehen
	//        cache_write_back()

	struct bundle_t *bundle = NULL;
	uint32_t bundle_number = 0;
	uint32_t bundle_persistent_address = CACHE_PARTITION_BUNDLES_INVALID_TAG;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "storage_cached_save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "storage_cached_save_bundle with invalid MMEM structure");
		return 0;
	}

	// Calculate the bundle number
	bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs, bundle->app_len);

//FIXME Look for duplicates in the storage


	if( last_index_entry == BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS ) { //FIXME kann dank segmentierung auch noch später auftreten
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot store bundle, no room");
		return 0;
	}

	// Now we have to update the pointer to our bundle, because MMEM may have been modified (freed) and thus the pointer may have changed
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	// calculate address
	bundle_persistent_address = bundle_number % (CACHE_PARTITION_BUNDLES_END - CACHE_PARTITION_BUNDLES_START );
	//struct cache_entry_t cache_block = BUNDLE_CACHE.cache_access_block(bundle_persistent_address);

	//FIXME Check auf Kollisionen
	uint8_t bundle_persistent_address_collision = TRUE;
	while(bundle_persistent_address_collision == TRUE){
	    bundle_persistent_address_collision = FALSE;
        for(i=0; i<BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS; ++i){
            if(temp_index_array_collision_check[i] == bundle_persistent_address){
                bundle_persistent_address_collision = TRUE;
                ++bundle_persistent_address;
                break;
            }
        }
	}

	uint8_t buffer[528];

	uint16_t i;
	for(i=0; i<528; i+=4){
		buffer[i]=bundle->bundle_num;
	}

	/* read page */
	//at45db_read_page_bypassed( bundle_persistent_address, 0, buffer, 528 );

	// save to flash
	at45db_write_buffer( bundle_persistent_address, buffer, 528 );
	at45db_buffer_to_page( bundle_persistent_address );

	// we copy the reference to the bundle, therefore we have to increase the reference counter
	bundle_increment(bundlemem); //FIXME
	bundles_in_storage++;

	// Set all required fields
	bundle->bundle_num = bundle_number;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu, Src %lu, Dest %lu, Seq %lu", bundle->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

	// Notify the statistics module
	storage_cached_update_statistics();

	// Add index entry for bundle
	storage_cached_add_index_entry(bundle_number, bundle->dst_node);

	// Now we have to (virtually) free the incoming bundle slot
	// This should do nothing, as we have incremented the reference counter before
	bundle_decrement(bundlemem);  //FIXME

	// Now copy over the STATIC pointer to the bundle number, so that
	// the caller can stick it into an event
	*bundle_number_ptr = &temp_index_array[last_index_entry].bundle_num; //FIXME ist später mehr Zufall, wenn das klappt...
	                                                                     //FIXME Pointer auf ID in Cacheblock?
	return 1;
}

/**
 * \brief deletes a bundle from storage
 * \param bundle_number bundle number to be deleted
 * \param reason reason code
 * \return 1 on success or 0 on error
 */
uint16_t storage_cached_delete_bundle(uint32_t bundle_number, uint8_t reason)
{
	return 1;
	//FIXME bundle_number auf Liste
	//      wenn Liste dadurch über Treshold -> Garbage Collection
	//      reason loggen
	//      evtl. Statusreport mit reason verschicken, dafür bräuchte man Bündeldaten
	//      reason auch merken, aber bündel ist logisch gesehen schon gelöscht, d.h. report dann zu spät
}

/**
 * \brief reads a bundle from storage
 * \param bundle_num bundle number to read
 * \return pointer to the MMEM struct, NULL on error
 */
struct mmem *storage_cached_read_bundle(uint32_t bundle_num)
{
    //FIXME das wollen wir machen...
    //bundlemem = bundle_recover_bundle(payload, length);

    struct bundle_t * bundle = NULL;

    // Look for the bundle we are talking about
//    for(suchen) {
//        bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
//
//        if( bundle->bundle_num == bundle_num ) {
//            break;
//        }
//    }

//    if( nix da ) {
//        LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Could not find bundle %lu in storage_mmem_read_bundle", bundle_num);
//        return 0;
//    }

//    if( bundle->size == 0 ) {
//        LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Found bundle %lu but file size is %u", bundle_num, entry->bundle->size);
//        return 0;
//    }

    // Someone requested the bundle, he will have to decrease the reference counter again
    bundle_increment(bundle);

    //FIXME soll das wirklich hier gemacht werden?
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

    return bundle;
	//FIXME berechnet Speicheradresse (Fkt)
	//      while (Cacheblock ID =! bundle_num)
	//        if (cache_access(Speicheradresse)) == NULL
	//          Cache voll: siehe save_bundle
	//          kann nicht passieren?
	//          nur wenn man speicher "blocken" will
	//        Speicheradresse++
	//      mmem-struct bauen + zurückgeben
	//      segmented-flag setzen
	//      merken, dass beim 2. read Speicheradresse+1 geliefert werden soll
	//      etc ...
}

/**
 * \brief checks if there is space for a bundle
 * \param bundlemem pointer to a bundle struct (not used here)
 * \return number of free slots
 */
uint16_t storage_cached_get_free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
	// FIXME gibt Anzahl freier Seiten zurück?
}

/**
 * \brief Get the number of slots available in storage
 * \returns the number of free slots
 */
uint16_t storage_cached_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \brief Get the bundle list
 * \returns pointer to first bundle list array in cache, sets index_array_entrys
 *
 * mode: head, next, free
 *
 */
//FIXME status
struct storage_index_entry_t * storage_cached_get_bundles(uint8_t mode, uint8_t *index_array_entrys){

    *index_array_entrys = last_index_entry; //FIXME Unterscheidung zwischen dem und BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS

    //FIXME simuliert Ende der Liste
	if(temp_index_array_toggle == 1){
	    *index_array_entrys = 0;
		temp_index_array_toggle = 0;
	} else {
		temp_index_array_toggle = 1;
	}

	/** Check mode, allow next-/free-mode only, if head was requested before */
	if ( mode == STORAGE_CACHED_GET_BUNDLES_HEAD && next_mode == STORAGE_CACHED_GET_BUNDLES_HEAD ){
		next_mode = STORAGE_CACHED_GET_BUNDLES_NEXT;
	} else if ( mode == STORAGE_CACHED_GET_BUNDLES_NEXT && next_mode == STORAGE_CACHED_GET_BUNDLES_NEXT ){
		next_mode = mode;
	} else if (mode == STORAGE_CACHED_GET_BUNDLES_FREE && next_mode == STORAGE_CACHED_GET_BUNDLES_NEXT ){
		//FIXME frees occupied cache blocks
		//FIXME simuliert zurücksetzten auf start
		temp_index_array_toggle = 0;
	} else {
		//FIXME Falscher mode, abbrechen
	}

	//get "next" index block from cache
	//struct cache_entry_t cache_block = BUNDLE_CACHE.cache_access_partition(CACHE_PARTITION_NEXT_BLOCK, CACHE_PARTITION_B_INDEX_START, last_index_block_address);

//    uint8_t i;
//    for(i=0; i<BUNDLE_STORAGE_INDEX_ENTRYS; ++i){
//    	temp_index_array[i].bundle_num=i+1; //FIXME ID
//    	temp_index_array[i].dst_node=i+1;  //FIXME Zielnode
//    }
//
//    //FIXME nur bei RAM-Block nötig
//    for(i=0; i<BUNDLE_STORAGE_INDEX_ENTRYS; ++i){
//        	if(temp_index_array[i].bundle_num == 0 && temp_index_array[i].dst_node == 0){
//        		*index_array_entrys = i-1;
//        		break;
//        	}
//    }
//
//
// TEST
//	int main(void){
//	        struct storage_index_entry_t *array_ptr = NULL;
//	        int array_length = 0;
//	        array_ptr = storage_cached_get_bundles(STORAGE_CACHED_GET_BUNDLES_HEAD, &array_length);
//
//	        int i;
//	        for(i=0; i<array_length; ++i){
//	                printf("Bundle[%d] : [ID] %d : [Dest] %d\n",i,array_ptr[i].bundle_num,array_ptr[i].dst_node);
//	        }
//	        return 0;
//	}

    return temp_index_array;

	//FIXME Beim 2. - n. Aufruf: Array 2 - n
	//      Dann 1mal länge 0
	//      Dann wieder von vorne
	//      Blockiert bis zum nächsten read den entsprechenden Cacheblock, d.h. Liste muss komplett durchlaufen werden
	//      state pro aufrufer?
	//      freigeben von liste?
	//      Kümmert sich um das Nachladen vom Flash

}

const struct storage_driver storage_cached = {
	"STORAGE_CACHED",
	storage_cached_init,
	storage_cached_reinit,
	storage_cached_save_bundle,
	storage_cached_delete_bundle,
	storage_cached_read_bundle,
	storage_cached_get_free_space,
	storage_cached_get_bundle_numbers,
	storage_cached_get_bundles,
};

/** @} */
/** @} */
