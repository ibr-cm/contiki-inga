/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup storage_persistent_flash Flash-based persistent storage-interface
 *
 * @{
 */

/**
 * \file 
 * \author Julian Heinbokel <j.heinbokel@tu-bs.de>
 */

#include "storage_persistent.h"

uint8_t storage_persistent_flash_write_block(uint16_t address, uint8_t *data, uint16_t length, uint16_t prev_segm, uint16_t next_segm){
	printf("persistent_storage_flash: write\n");
	return 0;
}

uint8_t storage_persistent_flash_read_block(uint16_t address, uint8_t *data, uint16_t length, uint16_t prev_segm, uint16_t next_segm){
	printf("persistent_storage_flash: read\n");
	return 0;
}

uint8_t storage_persistent_flash_flush(uint16_t start_address, uint16_t end_address){
    printf("persistent_storage_flash_flush: start_address: %u , end_address: %u\n", start_address, end_address);
    return 0;
}

const struct storage_persistent_driver storage_persistent_flash = {
	"PERSISTENT_STORAGE_FLASH",
	storage_persistent_flash_write_block,
	storage_persistent_flash_read_block,
    storage_persistent_flash_flush,
};

/** @} */
/** @} */
