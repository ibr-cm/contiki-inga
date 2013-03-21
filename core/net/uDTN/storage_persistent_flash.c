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

#include "flash-at45db.h"

#include "storage_persistent.h"

uint8_t storage_persistent_write_protected = 0;

uint8_t storage_persistent_flash_write_block(uint16_t address, uint8_t *data, uint16_t length, uint8_t *prev_segm, uint8_t *next_segm){
	printf("storage_persistent_flash_write_block: address: %u, data: %p, length: %u, prev_segm: %u, next_segm: %u\n", address, data, length, (uint16_t) *prev_segm, (uint16_t) *next_segm);

	if (!storage_persistent_write_protected){
	    printf("storage_persistent_flash_write_block: WRITING_TO_FLASH_ONCE!"); //FIXME
	    storage_persistent_write_protected = 1;
        /* write data-block to the beginning of the buffer */
        at45db_write_buffer(0, data, length);

        /* write prev&next-pointer to the last 4 bytes of the buffer */
        at45db_write_buffer(524, prev_segm, 2);
        at45db_write_buffer(526, next_segm, 2);

        /* write buffer to flash */
        at45db_buffer_to_page(address);
	}

	return 0;
}

uint8_t storage_persistent_flash_read_block(uint16_t address, uint8_t *data, uint16_t length, uint8_t *prev_segm, uint8_t *next_segm){
	printf("storage_persistent_flash_read_block\n");

	/* read page to buffer, write data-block to pointer */
	at45db_read_page_buffered(address, 0, data, length);

	/* get prev&next-pointer from buffer */
	at45db_read_buffer(524, prev_segm, 2);
	at45db_read_buffer(526,  next_segm, 2);

	return 0;
}

uint8_t storage_persistent_flash_delete_blocks(uint16_t start_address, uint16_t end_address){
    printf("persistent_storage_flash_flush: start_address: %u , end_address: %u\n", start_address, end_address);

    //FIXME add at45db_erase_block()
    if(start_address == 0 && end_address == 4095){
        at45db_erase_chip();
    } else {
        for(; start_address<=end_address; ++start_address){
            at45db_erase_page(start_address);
        }
    }
    return 0;
}

const struct storage_persistent_driver storage_persistent_flash = {
	"PERSISTENT_STORAGE_FLASH",
	storage_persistent_flash_write_block,
	storage_persistent_flash_read_block,
    storage_persistent_flash_delete_blocks,
};

/** @} */
/** @} */
