/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle_storage Persistent block storage interface
 *
 * @{
 */

/**
 * \file 
 * \brief this file defines the interface for persistent storage used by storage modules
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#ifndef __STORAGE_PERSISTENT_H__
#define __STORAGE_PERSISTENT_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "watchdog.h"

extern uint8_t storage_persistent_write_protected; //FIXME

/**
 * Which persistent storage driver are we going to use?
 * Default is Flash
 */
#ifdef CONF_STORAGE_PERSISTENT
#define STORAGE_PERSISTENT CONF_STORAGE_PERSISTENT
#else
#define STORAGE_PERSISTENT storage_persistent_flash
#endif

/** persistent storage module interface  */
struct storage_persistent_driver {
	char *name;
	/**
	 * \brief writes block of data to persistent storage
	 * \param address in persistent storage
	 * \param pointer to read data from
	 * \param lenght of data
	 * \param pointer to previous data segment in persistent storage (address)
	 * \param pointer to next data segment in persistent storage (address)
	 */
	uint8_t (* write_block)(uint16_t address, uint8_t *data, uint16_t length, uint16_t prev_segm, uint16_t next_segm); //FIXME konzeption sieht noch *bundlemem vor, scheint überflüssig...
	/**
	 * \brief reads block of data from persistent storage
	 * \param address in persistent storage
	 * \param pointer to write data to
	 * \param lenght of data
	 * \param pointer to previous data segment in persistent storage (address)
	 * \param pointer to next data segment in persistent storage (address)
	 */
	uint8_t (* read_block)(uint16_t address, uint8_t *data, uint16_t length, uint16_t *prev_segm, uint16_t *next_segm); //FIXME konzeption sieht noch *bundlemem vor, scheint überflüssig...
	/**
	 * \brief deletes contents of persistent storage
	 * \param address to start deletion
	 * \param last address to delete
	 */
	uint8_t (* delete_blocks)(uint16_t start_address, uint16_t end_address);
};
extern const struct storage_persistent_driver STORAGE_PERSISTENT;
#endif
/** @} */
/** @} */
