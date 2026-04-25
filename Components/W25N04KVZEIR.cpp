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

/**
 * @brief loop while flash busy, return false if timeout or HAL error occurs
 */
static bool W25N_ensure_ready(void) {
    uint32_t start = HAL_GetTick();
    while (true) {
        uint8_t status = W25N_status();
        if (status == 0xFF) return false;              // HAL error
        if (!(status & W25N_STATUS_BUSY)) return true; // chip is ready
        if ((HAL_GetTick() - start) >= W25N_TIMEOUT_MS) return false; // timeout
    }
}

/**
 * @brief read status register
 */
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

/**
 * @brief reset device
 */
uint8_t W25N_reset(void) {
    if (!W25N_ensure_ready()) {
        return 1;
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

    if (!W25N_ensure_ready()) return 1;
    return 0;
}

/**
 * @brief read jedec ID
 */
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
    
    if (!W25N_ensure_ready()) return 0xFFFFFFFF;
    return (read_data[0] << 16) | (read_data[1] << 8) | read_data[2];
}

/**
 * @brief read data starting from specified page and offset into buffer
 */
uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data) {
    if (!W25N_ensure_ready()) {
        return 1;
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

    if (!W25N_ensure_ready()) return 1;

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

    if (!W25N_ensure_ready()) return 1;
    return 0;
}

/**
 * @brief 128K block erase
 */
uint8_t W25N_block_erase(uint32_t block) {
    if (!W25N_ensure_ready()) return 1;

    if (clear_block_protection() != 0){
        return 1;
    }

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

    if (!W25N_ensure_ready()) return 1;
    return 0;
}

/**
 * @brief clear block protection bits to allow erasing/programming
 */
uint8_t clear_block_protection(void) {
    if (!W25N_ensure_ready()) return 1;

    // send write enable
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_WRITE_ENABLE;
    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    // write 0x00 to status register to clear BP bits
    uint8_t val = 0x00;
    cmd.Instruction     = W25N_CMD_WRITE_STATUS_REG;
    cmd.AddressMode     = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = QSPI_ADDRESS_8_BITS;
    cmd.Address         = W25N_SREG_PROTECTION;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;
    if (HAL_QSPI_Transmit(&hqspi1, &val, HAL_MAX_DELAY) != HAL_OK) return 1;

    if (!W25N_ensure_ready()) return 1;
    return 0;
}

/**
 * @brief write to flash at page + offset
 */
uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data) {
    if (!W25N_ensure_ready()) {
        return 1;
    }

    if (clear_block_protection() != 0){
        return 1;
    }

    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = W25N_CMD_WRITE_ENABLE;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK) return 1;

    osDelay(W25N_DELAY_RESET_MS);

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

    osDelay(W25N_DELAY_RESET_MS);

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

    if (!W25N_ensure_ready()) return 1;
    return 0;
}



