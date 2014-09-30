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

#ifndef RF231_H_
#define RF231_H_

#include "contiki.h"
#include "hal.h"
#include "string.h"

#define ADDR_AES_STATUS 0x82
#define ADDR_AES_CTRL   0x83
#define ADDR_AES_DATA   0x84
#define ADDR_AES_KEY    ADDR_AES_DATA

#define AES_REQUEST   7
#define AES_MODE      4
#define AES_DIRECTION 3

#define AES_REQUEST_IDLE  (0 << AES_REQUEST)
#define AES_REQUEST_WRITE (1 << AES_REQUEST)

#define AES_MODE_ECB (0 << AES_MODE)
#define AES_MODE_KEY (1 << AES_MODE)
#define AES_MODE_CBC (2 << AES_MODE)

#define AES_DIRECTION_ENCRYPTION (0 << AES_DIRECTION)
#define AES_DIRECTION_DECRYPTION (1 << AES_DIRECTION)
#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

extern const struct aes_128_driver rf231_aes_128_driver;
void rf231_aes_128_set_key(uint8_t *ciphertext_and_result, uint8_t decryption);
void rf231_aes_128_decrypt(uint8_t *ciphertext_and_result);
void rf231_aes_128_encrypt(uint8_t *ciphertext_and_result);

#endif /* RF231_H_ */