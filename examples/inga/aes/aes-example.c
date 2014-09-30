#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "clock.h"
#include <util/delay.h>
#include <string.h>
#include "lib/aes-128.h"
#include "dev/rf231/rf231.h"

uint8_t key[AES_KEY_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t buffer[AES_BLOCK_SIZE];

/*---------------------------------------------------------------------------*/
void
show_block(uint8_t *data)
{
  uint8_t i;
  for(i = 0; i < AES_BLOCK_SIZE; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Set AES-Key (contiki): ");
  show_block(key);
  AES_128.set_key(key);

  memset(buffer, 0x12, AES_BLOCK_SIZE);
  printf("Encrypt: ");
  show_block(buffer);
  AES_128.encrypt(buffer);
  printf("Result: ");
  show_block(buffer);

  printf("Set AES-Key (rf231 specific): ");
  show_block(key);
  rf231_aes_128_set_key(key, 0);

  memset(buffer, 0x12, AES_BLOCK_SIZE);
  printf("Encrypt: ");
  show_block(buffer);
  rf231_aes_128_encrypt(buffer);
  printf("Result: ");
  show_block(buffer);

  printf("Setup Decryption\n");
  rf231_aes_128_set_key(key, 1);

  printf("Decrypt: ");
  show_block(buffer);
  rf231_aes_128_decrypt(buffer);
  printf("Result: ");
  show_block(buffer);

  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
