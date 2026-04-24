/*
 * W25N04KVZEIR.cpp
 *
 *  Created on: Apr 12, 2026
 *      Author: Christy
 */

#include "main.h"
#include "W25N04KVZEIR.hpp"
#include "CubeTask.hpp"

extern "C" {
    extern QSPI_HandleTypeDef hqspi1;
}

// --- Instruction Set ---
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

// --- Status Register Addresses ---
#define W25N_SREG_STATUS            0xC0

// --- Timing ---
#define W25N_DELAY_RESET_MS         5     // tRST max 500us
#define W25N_DELAY_BLOCK_ERASE_MS   10    // tBE max 10ms

// --- Status Bits ---
#define W25N_STATUS_BUSY            0x01


#define W25N_TIMEOUT_MS  500



static bool W25N_ensure_ready(void) {
    uint32_t start = HAL_GetTick();
    while (true) {
        uint8_t status = W25N_status();
        if (status == 0xFF) return false;              // HAL error
        if (!(status & W25N_STATUS_BUSY)) return true; // chip is ready
        if ((HAL_GetTick() - start) >= W25N_TIMEOUT_MS) return false; // timeout
    }
}


uint8_t W25N_status(void) {
    QSPI_CommandTypeDef cmd = {0};
    uint8_t status = 0;

    cmd.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction         = W25N_CMD_READ_STATUS_REG;
    cmd.AddressMode         = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode   = QSPI_ALTERNATE_BYTES_1_LINE;
    cmd.AlternateBytesSize  = QSPI_ALTERNATE_BYTES_8_BITS;
    cmd.AlternateBytes      = W25N_SREG_STATUS;
    cmd.DataMode            = QSPI_DATA_1_LINE;
    cmd.NbData              = 1;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 0xFF;

    if (HAL_QSPI_Receive(&hqspi1, &status, HAL_MAX_DELAY) != HAL_OK) 
        return 0xFF;

    return status;
}


void W25N_wait_ready(void) {
    uint8_t status;
    do {
        status = W25N_status();
    } while (status & W25N_STATUS_BUSY);
}


uint8_t W25N_reset(void) {
    if (!W25N_ensure_ready()) {
        return 0xFF;
    }

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;

    cmd.Instruction = W25N_CMD_RESET_ENABLE;
    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 1;

    cmd.Instruction = W25N_CMD_RESET;
    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 1;

    osDelay(W25N_DELAY_RESET_MS);
    return 0;
}


uint32_t W25N_read_id(void) {
    if (!W25N_ensure_ready()) {
        return 0xFFFFFFFF;
    }

    QSPI_CommandTypeDef cmd = {0};
    uint8_t read_data[3];

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_READ_JEDEC_ID;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.DummyCycles     = 8;
    cmd.NbData          = 3;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 0xFFFFFFFF;

    if (HAL_QSPI_Receive(&hqspi1, read_data, HAL_MAX_DELAY) != HAL_OK) 
        return 0xFFFFFFFF;
    
    osDelay(W25N_DELAY_RESET_MS);
    return (read_data[0] << 16) | (read_data[1] << 8) | read_data[2];
}


uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data) {
    if (!W25N_ensure_ready()) {
        return 0xFF;
    }

    // Load page into cache
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_PAGE_DATA_READ;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_24_BITS;
    cmd.Address         = start_page;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    // Read from cache buffer
    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_FAST_READ_QUAD;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_16_BITS;
    cmd.Address         = offset;
    cmd.DataMode        = QSPI_DATA_4_LINES;
    cmd.DummyCycles     = 8;
    cmd.NbData          = size;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 1;

    if (HAL_QSPI_Receive(&hqspi1, data, HAL_MAX_DELAY) != HAL_OK) 
        return 1;

    osDelay(W25N_DELAY_RESET_MS);
    return 0;
}


uint8_t W25N_block_erase(uint32_t block) {
    if (!W25N_ensure_ready()) return 0xFF;

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_WRITE_ENABLE;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_BLOCK_ERASE;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_24_BITS;
    cmd.Address         = block * 64;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    HAL_Delay(W25N_DELAY_BLOCK_ERASE_MS);
    return 0;
}


uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data) {
    if (!W25N_ensure_ready()) {
        return 1;
    }

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_WRITE_ENABLE;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    // Load data into cache buffer
    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_QUAD_LOAD_RANDOM;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_16_BITS;
    cmd.Address         = offset;
    cmd.DataMode        = QSPI_DATA_4_LINES;
    cmd.NbData          = size;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;
    if (HAL_QSPI_Transmit(&hqspi1, data, HAL_MAX_DELAY) != HAL_OK) return 1;

    // Commit cache to flash
    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_PROGRAM_EXECUTE;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_24_BITS;
    cmd.Address         = page;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) 
        return 1;

    osDelay(W25N_DELAY_RESET_MS);
    return 0;
}
