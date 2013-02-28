/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle_storage Bundle Storage modules
 *
 * @{
 */

/**
 * \file 
 * \brief this file defines the interface for storage modules
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "memb.h"

#include "bundle.h"
#include "bundleslot.h"  //FIXME ist das notwendig?

#define STORAGE_FIRST_SEGMENT 0x01
#define STORAGE_SEGMENT       0x02
#define STORAGE_LAST_SEGMENT  0x03
#define STORAGE_NO_SEGMENT    0x04

/**
 * Which storage driver are we going to use?
 * Default is RAM
 */
#ifdef BUNDLE_CONF_STORAGE
#define BUNDLE_STORAGE BUNDLE_CONF_STORAGE
#else
#define BUNDLE_STORAGE storage_mmem
#endif

/**
 * How many bundles can possibly be stored in the data structures?
 */
#ifdef BUNDLE_CONF_STORAGE_SIZE
#define BUNDLE_STORAGE_SIZE BUNDLE_CONF_STORAGE_SIZE
#else
#define BUNDLE_STORAGE_SIZE 	10
#endif

/**
 * Should storage go into an initial safe state when starting up?
 * Otherwise, some storages may try to reconstruct the last start before powering down
 */
#ifdef BUNDLE_CONF_STORAGE_INIT
#define BUNDLE_STORAGE_INIT BUNDLE_CONF_STORAGE_INIT
#else
#define BUNDLE_STORAGE_INIT 0
#endif

#define BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS 66 //FIXME = floor(MAX_SLOT_SIZE / sizeof(struct bundle_index_entry_t))
/**
 * Representation of a bundle in the array returned by the "get_index_block" call to the storage module
 */
struct bundle_index_entry_t { //FIXME nach bundle.h
    /** Internal number of the bundle */
    uint32_t bundle_num;
    
    /** Destination node of the bundle */
    uint32_t dst_node;
};

//FIXME
struct storage_entry_t {
       /** pointer to the next list element */
       struct storage_entry_t * next;

       /** Internal number of the bundle */
       uint32_t bundle_num;
};

/** storage module interface  */
struct storage_driver {
	char *name;
	/**
	 * \brief called by agent a startup, builds index, does all needed housekeeping
	 *
	 * after this, the storage is considered to be in an clean state
	 */
	uint8_t (* init)(void);
	/**
	 * \brief deletes all stored bundles + index blocks
	 */
	uint8_t (* flush)(void); //FIXME vorher reinit
	/**
	 * \brief tells storage to do housekeeping for <time> ms
     * \param time available
     * \return 0 on error, 1 on success, 2 on more housekeeping needed
	 *
	 * does the following, as long as there is time:
	 *     makes non-persistent storage segments persistent
	 * when there is time left:
	 *     deletes bundles from FIXME <delete_list>
     * when there is time left:
	 *     scans persistent storage for bundles to delete, puts them on <delete_list>
	 */
    uint8_t (* housekeeping)(uint16_t time);
	//void (* make_all_persistent)(void); //FIXME ist das wirklich notwendig?
	//void (* garbage_collect)(uint8_t time); //FIXME
    /**
     * \brief storage may never release bundleslots once they are allocated, this trys to release bundleslots to get size free bytes
     * \param size in bytes
     * \return 0 on error, 1 on success
     */
    uint8_t (* release_bundleslots)(uint16_t size);
	/**
	 * \brief puts bundle in storage, creates index entry
	 * \param pointer to bundle struct
     * \param segmentation flag: STORAGE_FIRST_SEGMENT, STORAGE_SEGMENT, STORAGE_LAST_SEGMENT, STORAGE_NO_SEGMENT
     * \return 0 on error, 1 on success
	 *
	 * creates events: dtn_bundle_stored, dtn_bundle_in_storage_event, dtn_bundle_store_failed, ...  //FIXME
	 */
	 uint8_t (* save_bundle)(struct mmem *bundlemem, uint8_t flags);
    /**
     * \brief add another segment to bundlemem
     * \param pointer to bundle struct which needs more memory
     * \param minimum size needed
     * \return 0 on error, 1 on success
     *
     * reserves another bundleslot if necessary, can accumulate min_size of multiple calls to save bundleslots
     */
    uint8_t (* add_segment_to_bundle)(struct mmem *bundlemem, uint16_t min_size);
    /**
     * \brief deletes a bundle
     * \param pointer to BundleID
     * \return 0 on error, 1 on success
     *
     * marks bundle for garbage collection
     */
	//FIXME sollte Statusreport nicht besser vom Aufrufer verschickt werden? (Stichwort: REASON_DELIVERED)
	uint8_t (* del_bundle_by_bundle_number)(uint32_t bundle_num);
    /**
     * \brief deletes a bundle
     * \param pointer to index entry
     * \return 0 on error, 1 on success
     *
     * deletes index entry directly
     * marks bundle for garbage collection
     */
	uint8_t (* del_bundle_by_index_entry)(struct bundle_index_entry_t *index_entry);
    /**
     * \brief reads a bundle
     * \param pointer to BundleID
     * \param offset in block data
     * \param length of requested block data
     * \return pointer to bundle struct
     *
     * returns bundle struct, sets/changes the block_data field so it starts at block_data_start_offset and contains at least block_data_length bytes
     * for block_data_start_offset = block_data_length, the block_data field is not touched if already in memory, else it contains the first block
     */
	//FIXME storage muss sich merken: block_data_start_offset,
	                                //länge des belegten speicherplatzes in block_data
	                                //cache flags
	struct mmem *(* read_bundle)(uint32_t bundle_num, uint32_t block_data_start_offset, uint16_t block_data_length);
    /**
     * \brief returns number of free bundle slots in storage, multiply with DATA_BLOCK_SIZE for Bytes
     * \return number of free bundle slots in storage
     */
	uint32_t (* get_free_space)(void);
	/** returns the number of saved bundles */
	uint16_t (* get_bundle_count)(void);
    /**
     * \brief get pointer to index array contained in bundleslot
     * \param block nr
     * \return NULL on error, pointer to index array storage_index_entry_t
     *
     * block 0 is always in memory, blocks 1 to BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS can be swapped to persistent storage
     * call bundle_increment(index_block) to protect indexblock from swapping, the block has to be released via bundle_decrement(index_block) afterwards
     */
    struct mmem *(* get_index_block)(uint8_t blocknr);
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
/** @} */
/** @} */
