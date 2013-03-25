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

#define LOW_BYTE(x) (x & 0xff)
#define HIGH_BYTE(x) ((x >> 8) & 0xff)

uint8_t storage_persistent_write_protected = 0; //FIXME

uint8_t storage_persistent_flash_write_block(uint16_t address, uint8_t *data, uint16_t length, uint16_t prev_segm, uint16_t next_segm){
	printf("storage_persistent_flash_write_block: address: %u, data: %p, length: %u, prev_segm: %u, next_segm: %u\n", address, data, length, prev_segm, next_segm);

	uint8_t buffer[4];

	if (!storage_persistent_write_protected){ //FIXME
	    printf("storage_persistent_flash_write_block: WRITING_TO_FLASH_ONCE!\n");
	    storage_persistent_write_protected = 1;

        /* write data-block to the beginning of the buffer */
        at45db_write_buffer(0, data, length);

        /* write prev&next-pointer to the last 4 bytes of the buffer */
        buffer[0] = HIGH_BYTE(prev_segm);
        buffer[1] = LOW_BYTE(prev_segm);
        buffer[2] = HIGH_BYTE(next_segm);
        buffer[3] = LOW_BYTE(next_segm);
        at45db_write_buffer(524, buffer, 4);

        /* write buffer to flash */
        at45db_buffer_to_page(address);
	}

	return 0;
}

//FIXME lenght: auch offset speichern?, ansonsten muss maximallänge genommen und weiter "oben" gekürzt werden
uint8_t storage_persistent_flash_read_block(uint16_t address, uint8_t *data, uint16_t length, uint16_t *prev_segm, uint16_t *next_segm){
	uint8_t buffer[4] = {0};

	/* read page to buffer, write data-block to pointer */
	at45db_read_page_buffered(address, 0, data, length);

	/* get prev&next-pointer from buffer */
	at45db_read_buffer(524, buffer, 4);
	*prev_segm = ((buffer[0] << 8) | buffer[1]);
    *next_segm = ((buffer[2] << 8) | buffer[3]);

    printf("storage_persistent_flash_read_block: address: %u, data: %p, length: %u, prev_segm: %u, next_segm: %u\n", address, data, length, *prev_segm, *next_segm);

	return 0;
}

uint8_t storage_persistent_flash_delete_blocks(uint16_t start_address, uint16_t end_address){
    printf("storage_persistent_flash_delete_blocks: start_address: %u , end_address: %u\n", start_address, end_address);

    //FIXME add at45db_erase_block()
    if(start_address == 0 && end_address == 4095){
        /* this will take ~20 sec, the watchdog would intercept us */
        watchdog_stop();
        at45db_erase_chip();
        watchdog_start();
    } else {
        for(; start_address<=end_address; ++start_address){
            at45db_erase_page(start_address);
            //FIXME watchdog_periodic();
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
