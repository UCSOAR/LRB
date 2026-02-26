/*
 * i2c_wrapper.cpp
 */
#include "i2c_wrapper.hpp"

I2C_Wrapper::I2C_Wrapper(I2C_HandleTypeDef* hi2c) : _hi2c(hi2c) {
}

bool I2C_Wrapper::writeByte(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t data) { 
    return (HAL_I2C_Mem_Write(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) == HAL_OK);
    
}

bool I2C_Wrapper::readByte(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t* dest) {
    return (HAL_I2C_Mem_Read(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT, dest, 1, 100) == HAL_OK);
}

bool I2C_Wrapper::readBytes(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t *data, std::uint8_t len) { 
    return HAL_I2C_Mem_Read(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK;
}