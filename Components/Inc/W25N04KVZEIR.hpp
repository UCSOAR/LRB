/*
 * W25N04KVZEIR.hpp
 *
 *  Created on: Apr 12, 2026
 *      Author: Christy
 */

#ifndef INC_W25N04KVZEIR_HPP_
#define INC_W25N04KVZEIR_HPP_

#include <stdint.h>
#include <cstring>

uint8_t W25N_reset(void);

uint32_t W25N_read_id(void);

uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data);

uint8_t W25N_block_erase(uint32_t block);

uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data);

uint8_t W25N_status(void);

void W25N_wait_ready(void);

// wrapping it in extern C so I can call it from main.c
// #ifdef __cplusplus
// extern "C" {
// #endif

// uint8_t W25N_reset(void);
// uint32_t W25N_read_id(void);
// uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data);
// uint8_t W25N_block_erase(uint32_t block);
// uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data);

// uint8_t W25N_status(void);
// #ifdef __cplusplus
// }
// #endif



#endif /* INC_W25N04KVZEIR_HPP_ */
