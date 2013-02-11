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
	//struct mmem *bundle;
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
 * \brief deletes expired bundles from storage
 */
void storage_cached_prune()
{
	//FIXME überprüft X Flashseiten auf abgelaufene Bündel
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
uint8_t storage_cached_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr)
{
	return 1;
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
	return entry->bundle;
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
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * storage_cached_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
	//FIXME Gibt Array + Arraylänge zurück
	//      Mit ID + Zielnode
	//      Beim 2. - n. Aufruf: Array 2 - n
	//      Dann 1mal NULL
	//      Dann wieder von vorne
	//      Blockiert bis zum nächsten read den entsprechenden Cacheblock, d.h. Liste muss komplett durchlaufen werden
	//      state pro aufrufer?
	//      freigeben von liste?
	//      Kümmert sich um das Nachladen vom Flash
	//      int index_newest_entry
	//      * index_newest_block
	//      storage_cached_add_index_entry(ID, TargetNode)
	//      storage_cached_del_index_entry(ID)
	//      storage_cached_build_index()
	//      storage_cached_get_index_block()
	//
	//      cache_get_list ( partition )
	//
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
