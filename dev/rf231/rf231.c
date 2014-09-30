/*
 * Copyright (c) 2014, TU Braunschweig
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file Additional functions for the extended features of RF231
 * \author Sebastian Willenborg <s.willenborg@tu-bs.de>
 */
#include "rf231.h"

uint8_t aes_buffer[18];
/*---------------------------------------------------------------------------*/
/**
 * \brief Waits until the aes-component completet its aes-operation
 */
void
rf231_aes_128_wait_until_is_ready()
{
  uint8_t status = 0;
  while(!status) {
    _delay_us(20);
    hal_sram_read(ADDR_AES_STATUS, 1, &status);
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief              Sets the aes-key for encyption or decryption
 * \param key          Pointer to the aes-key
 * \param decryption   Is 0 to setup encryption, 1 to setup decryption
 */
void
rf231_aes_128_set_key(uint8_t *key, uint8_t decryption)
{
  /* Set aes-key on RF231 */
  aes_buffer[0] = AES_MODE_KEY;
  memcpy(aes_buffer + 1, key, 16);
  hal_sram_write(ADDR_AES_CTRL, 1 + AES_KEY_SIZE, aes_buffer);

  if(decryption) {
    /* Perform dummy encryption to generate the "decryption" key */
    aes_buffer[0] = AES_MODE_ECB;
    memset(aes_buffer + 1, 0, 16);
    aes_buffer[17] = AES_MODE_ECB | AES_REQUEST_WRITE;
    hal_sram_write(ADDR_AES_CTRL, 1 + AES_BLOCK_SIZE + 1, aes_buffer);

    rf231_aes_128_wait_until_is_ready();

    /* Request the decryption key */
    aes_buffer[0] = AES_MODE_KEY;
    hal_sram_write(ADDR_AES_CTRL, 1, aes_buffer);

    /* Read the decryption key */
    hal_sram_read(ADDR_AES_KEY, AES_KEY_SIZE, aes_buffer + 1);

    /* Set the decryption key on RF231 */
    aes_buffer[0] = AES_MODE_KEY;
    hal_sram_write(ADDR_AES_CTRL, 1 + AES_KEY_SIZE, aes_buffer);
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Sets the aes-key for encryption
 * \param key   Pointer to the aes-key
 */
static void
rf231_aes_128_set_key_encryption(uint8_t *key)
{
  rf231_aes_128_set_key(key, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief                        Performs the encryption on the given block
 * \param plaintext_and_result   Pointer to an buffer, which will be replaced
 *                               with its encrypted content (16 Bytes)
 */
void
rf231_aes_128_encrypt(uint8_t *plaintext_and_result)
{
  aes_buffer[0] = AES_DIRECTION_ENCRYPTION | AES_MODE_ECB;
  memcpy(aes_buffer + 1, plaintext_and_result, AES_BLOCK_SIZE);
  aes_buffer[1 + AES_BLOCK_SIZE] = AES_DIRECTION_ENCRYPTION | AES_MODE_ECB | AES_REQUEST_WRITE;
  hal_sram_write(ADDR_AES_CTRL, 1 + AES_BLOCK_SIZE + 1, aes_buffer);

  rf231_aes_128_wait_until_is_ready();

  hal_sram_read(ADDR_AES_CTRL, 1 + AES_BLOCK_SIZE, aes_buffer);
  memcpy(plaintext_and_result, aes_buffer + 1, AES_BLOCK_SIZE);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief                        Performs the decryption on the given block
 * \param plaintext_and_result   Pointer to an buffer, which will be replaced
 *                               with its decrypted content (16 Bytes)
 */
void
rf231_aes_128_decrypt(uint8_t *ciphertext_and_result)
{
  aes_buffer[0] = AES_DIRECTION_DECRYPTION | AES_MODE_ECB;
  memcpy(aes_buffer + 1, ciphertext_and_result, AES_BLOCK_SIZE);
  aes_buffer[1 + AES_BLOCK_SIZE] = AES_DIRECTION_DECRYPTION | AES_MODE_ECB | AES_REQUEST_WRITE;
  hal_sram_write(ADDR_AES_CTRL, 1 + AES_BLOCK_SIZE + 1, aes_buffer);

  rf231_aes_128_wait_until_is_ready();

  hal_sram_read(ADDR_AES_CTRL, AES_BLOCK_SIZE + 1, aes_buffer);
  memcpy(ciphertext_and_result, aes_buffer + 1, AES_BLOCK_SIZE);
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver rf231_aes_128_driver = {
  rf231_aes_128_set_key_encryption,
  rf231_aes_128_encrypt
};