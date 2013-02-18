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
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "memb.h"

#include "bundle.h"

#define BUNDLE_STORAGE_INDEX_ARRAY_ENTRYS 66
#define BUNDLE_STORAGE_CONCURRENT_BUNDLES CACHE_BLOCKS_NUM //FIXME ist das BUNDLE_NUM aus bundleslot.c ?

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
 * How do we call the bundle list representation file (if applicable)
 */
#define BUNDLE_STORAGE_FILE_NAME "list_file"

/**
 * Should storage go into an initial safe state when starting up?
 * Otherwise, some storages may try to reconstruct the last start before powering down
 */
#ifdef BUNDLE_CONF_STORAGE_INIT
#define BUNDLE_STORAGE_INIT BUNDLE_CONF_STORAGE_INIT
#else
#define BUNDLE_STORAGE_INIT 0
#endif

/**
 * Representation of a bundle in the array returned by the "get_index_block" call to the storage module
 */
struct storage_index_entry_t {
    /** Internal number of the bundle */
    uint32_t bundle_num;
    
    /** Destination node of the bundle */
    uint32_t dst_node;
} __attribute__ ((packed));

/** storage module interface  */
struct storage_driver {
	char *name;
	/** called by agent a startup */
	void (* init)(void);
	void (* reinit)(void); //FIXME clear_storage() ?
    //FIXME
    struct bundle_slot_t *get_bundleslot(); //FIXME recycle used bundleslot
    void free_bundleslot();                 //FIXME release mmem space for other uses
	/**
	 * \brief calculates BundleID
	 * \param pointer to bundle struct
	 * \return BundleID
	 */
    //FIXME dispatching_check_report berechnet HASH selbst
	uint32_t (* get_bundle_num)(struct mmem *bundlemem);
	/**
	 * \brief saves a bundle
	 * \param pointer to bundle struct
     * \param pointer for BundleID
	 * \return 0 on error, 1 on success
	 */
	uint8_t (* save_bundle)(struct mmem *bundlemem, uint32_t ** bundle_number);
    /**
     * \brief appends data to stored bundle
     * \param pointer to BundleID
     * \param cl flags for segmentation (first, last)
     * \param pointer to bundle data
     * \param bundle data length
     * \return 0 on error, 1 on success
     */
    uint8_t (* append_to_bundle)(uint32_t ** bundle_number, uint8_t flags , uint8_t *data, uint16_t data_length);
    /**
     * \brief deletes a bundle
     * \param BundleID
     * \param pointer to index entry, NULL if unknown (use case: received status report)
     * \return 0 on error, 1 on success
     */
	//FIXME sollte Statusreport nicht besser vom Aufrufer verschickt werden?
	uint8_t (* del_bundle)(uint32_t bundle_num, struct storage_index_entry_t *index_entry);
    /**
     * \brief reads a bundle
     * \param BundleID
     * \return pointer to bundle struct
     */
	//FIXME mode:head, next, free
	struct mmem *(* read_bundle)(uint32_t bundle_num); //FIXME pointer um stackspeicher zu sparen?
    /**
     * \brief reads bundle payload
     * \param BundleID
     * \param start offset in payload
     * \param end offset in payload
     * \param pointer for data
     * \return 0 on error, 1 on success, 2 on more data available (segmentation)
     */
    uint8_t (* read_bundle_data)(uint32_t bundle_num, uint32_t start_offset, uint32_t end_offset, uint8_t *data); //FIXME pointer um stackspeicher zu sparen?
	//FIXME oder so: read_bundle_done(ID)
    /**
     * \brief returns number of free bundle slots in storage, multiply with DATA_BLOCK_SIZE for Bytes
     * \return number of free bundle slots in storage
     */
	uint32_t (* get_free_space)(void);
	/** returns the number of saved bundles */
	uint16_t (* get_bundle_count)(void);
    /**
     * \brief opens bundle index session
     * \return 0 on no session free, session id
     */
	uint8_t (* get_index_session)(void);
    /**
     * \brief get number of index arrays
     * \param session id
     * \return number of index blocks
     */
    uint8_t (* get_index_length)(uint8_t sessionid);
    /**
     * \brief get pointer to index array
     * \param session id
     * \param entry nr
     * \return NULL on error, pointer to index array
     */
    struct storage_index_entry_t *(* get_index_block)(uint8_t sessionid, uint8_t blocknr);
    /**
     * \brief frees memory used by index array
     * \param session id
     * \param entry nr
     * \return 0 on error, 1 on success
     */
    uint8_t (* free_index_entry)(uint8_t sessionid, uint8_t blocknr);

	//FIXME irgendwas das die Garbage Collection veranlasst
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
/** @} */
/** @} */
