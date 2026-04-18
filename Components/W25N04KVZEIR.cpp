/*
 * W25N04KVZEIR.cpp
 *
 *  Created on: Apr 12, 2026
 *      Author: Christy
 */

#include "main.h"
#include "W25N04KVZEIR.hpp"

// define qspi handle
extern "C" {
extern QSPI_HandleTypeDef hqspi1;
}


uint8_t W25N_reset(void) {
    QSPI_CommandTypeDef cmd = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.DataMode = QSPI_DATA_NONE;

    cmd.Instruction = 0x66;     // enable reset
    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    cmd.Instruction = 0x99;     // reset
    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    HAL_Delay(5);             // delay to allow full reset (tRST is max 500ums)
    return 0;
}


uint32_t W25N_read_id(void) {
    QSPI_CommandTypeDef cmd = {0};
    uint8_t read_data[3];

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.DummyCycles = 8;
    cmd.NbData = 3;
    cmd.Instruction = 0x9F;     // read JEDEC id

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 0xFFFFFFFF;

    if (HAL_QSPI_Receive(&hqspi1, read_data, HAL_MAX_DELAY) != HAL_OK)
        return 0xFFFFFFFF;

    return (read_data[0] << 16) | (read_data[1] << 8) | read_data[2];
}


uint8_t W25N_read(uint32_t start_page, uint16_t offset, uint32_t size, uint8_t *data) {

    // load flash page
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.Instruction = 0x13;
    cmd.Address = start_page;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    // HAL_Delay(5);

    // read flash segment
    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_16_BITS;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.DummyCycles = 8;
    cmd.NbData = size;
    cmd.Instruction = 0x6B;
    cmd.Address = offset;

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    if (HAL_QSPI_Receive(&hqspi1, data, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    return 0;
}


uint8_t W25N_block_erase(uint32_t block) {
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_NONE ;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.Instruction = 0x06;     // write enable

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = block * 64;
    cmd.Instruction = 0xD8;     // block reset

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    HAL_Delay(10);  // delay to allow full reset (tBE is max 10ms)
    return 0;
}


uint8_t W25N_program_data(uint32_t page, uint16_t offset, uint16_t size, uint8_t *data) {
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_NONE ;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.Instruction = 0x06;     // write enable

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.AddressSize = QSPI_ADDRESS_16_BITS;
    cmd.Address = offset;
    cmd.NbData = size;
    cmd.Instruction = 0x34;     // Quad Random Load Program Data

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    if (HAL_QSPI_Transmit(&hqspi1, data, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    cmd = {0};
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.Address = page;
    cmd.Instruction = 0x10;     // program execute

    if (HAL_QSPI_Command(&hqspi1, &cmd, HAL_MAX_DELAY) != HAL_OK)
        return 1;
    
    return 0;
}