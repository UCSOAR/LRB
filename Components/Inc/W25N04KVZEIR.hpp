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

// instructions
#define W25N_CMD_READ_STATUS_REG    0x0F
#define W25N_CMD_RESET_ENABLE       0x66
#define W25N_CMD_RESET              0x99
#define W25N_CMD_READ_JEDEC_ID      0x9F
#define W25N_CMD_PAGE_DATA_READ     0x13
#define W25N_CMD_FAST_READ_QUAD     0x6B
#define W25N_CMD_WRITE_ENABLE       0x06
#define W25N_CMD_BLOCK_ERASE        0xD8
#define W25N_CMD_QUAD_LOAD_RANDOM   0x34
#define W25N_CMD_PROGRAM_EXECUTE    0x10
#define W25N_CMD_WRITE_STATUS_REG   0x1F
#define W25N_SREG_PROTECTION        0xA0

// status register address and busy bit
#define W25N_SREG_STATUS            0xC0
#define W25N_STATUS_BUSY            0x01

// timing
#define W25N_DELAY_RESET_MS         5     // tRST max 500us
#define W25N_TIMEOUT_MS             500   // tTIMEOUT max 500ms 

// function prototypes
uint8_t W25N_reset(void);

uint32_t W25N_read_id(void);

uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data);

uint8_t W25N_block_erase(uint32_t block);

uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data);

uint8_t W25N_status(void);

void W25N_wait_ready(void);

uint8_t clear_block_protection(void);

#endif /* INC_W25N04KVZEIR_HPP_ */
