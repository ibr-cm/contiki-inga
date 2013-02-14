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

// FIXME: löschen
/**
 * Representation of a bundle as returned by the "get_bundles" call to the storage module
 */
struct storage_entry_t {
	/** pointer to the next list element */
	struct storage_entry_t * next;

	/** Internal number of the bundle */
	uint32_t bundle_num;
};

/**
 * Representation of a bundle in the array returned by the "get_bundles" call to the storage module
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
	void (* reinit)(void);
	/** saves a bundle */
	uint8_t (* save_bundle)(struct mmem *bundlemem, uint32_t ** bundle_number);
	/** deletes a bundle */ //FIXME sollte Statusreport nicht besser vom Aufrufer verschickt werden?
	uint16_t (* del_bundle)(uint32_t bundle_num, uint8_t reason);
	/** reads a bundle */ //FIXME mode:head, next, free
	struct mmem *(* read_bundle)(uint32_t bundle_num);
	//FIXME oder so: read_bundle_done(ID)
	/** checks if there is space for a bundle */
	uint16_t (* free_space)(struct mmem *bundlemem);
	/** returns the number of saved bundles */
	uint16_t (* get_bundle_num)(void);
	/** returns pointer to list of bundles */ //FIXME Rückgabewert, evtl. Parameter
	struct storage_entry_t * (* get_bundles)(void);
	//FIXME oder so: get_bundles_reset(), get_bundles_get_length()
	//FIXME
	//irgendwas das die Garbage Collection veranlasst
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
/** @} */
/** @} */
