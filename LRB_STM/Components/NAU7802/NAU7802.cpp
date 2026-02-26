/**
 ********************************************************************************
 * @file    nau7802.cpp
 * @author  Javier
 * @date    2025-11-25
 * @brief   Implementation of the NAU7802 driver.
 ********************************************************************************
 */

#include "NAU7802.hpp"
#include "main.h"
#include <cstdint>

NAU7802::NAU7802(I2C_Wrapper* i2c_pointer, std::uint8_t address)
  : _i2c(i2c_pointer), _deviceAddress(address) {}

// Begin NAU7802 Startup Sequence
NauStatus NAU7802::begin(uint8_t initialGain) {

    // 1. Send a reset command
    if (reset() != NauStatus::OK) {
        return NauStatus::ERR_I2C; // Failed to reset
    }
    HAL_Delay(10);

    // 2. Power up the analog and digital sections
    uint8_t pu_ctrl = NAU7802_PU_CTRL_PUA | NAU7802_PU_CTRL_PUD;
    writeRegister(NAU7802_REG_PU_CTRL, pu_ctrl);

    // 3. Wait for the Power Up Ready bit
    uint8_t attempts = 100;
    bool powerReady = false;
    while (attempts > 0) {
        if (readRegister(NAU7802_REG_PU_CTRL, &pu_ctrl) == NauStatus::OK) {
            if (pu_ctrl & NAU7802_PU_CTRL_PUR) {
                powerReady = true;
                break;
            }
        }
        HAL_Delay(1);
        attempts--;
    }
    return setGain(initialGain);
}

bool NAU7802::isReady() {
    uint8_t status;
    if (readRegister(NAU7802_REG_PU_CTRL, &status) == NauStatus::OK) {
        return (status & NAU7802_PU_CTRL_CR) != 0;
    }
    return false;
}

NauStatus NAU7802::readSensor(NAU7802_OUT *dest) {
    uint8_t buffer[3];

    if (readRegisters(NAU7802_REG_ADC_B2, buffer, 3 ) != NauStatus::OK) {
        dest -> raw_reading = 0;
        return NauStatus::ERR_I2C; // Read failed
    }

    // Combine the three bytes
    int32_t value = ((int32_t)buffer[0] << 16) | \
                    ((int32_t)buffer[1] << 8)  | \
                    (buffer[2]);

    // Sign-extend the 24-bit value to a 32-bit integer
    if (value & 0x00800000) {
        value |= 0xFF000000;
    }

    dest->raw_reading = value;
    return NauStatus::OK;
}

NauStatus NAU7802::reset() {
    // RR bit
    modifyRegisterBit(NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_RR, true);

    HAL_Delay(1); // Small delay to ensure reset is processed

    // Clear RR bit
    return modifyRegisterBit(NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_RR, false);
}

NauStatus NAU7802::setGain(uint8_t gain) {
    if (gain > NAU7802_GAIN_128X) {
        return NauStatus::ERR_INVALID_ARG; // Invalid gain setting
    }

    // Read current CTRL1 register state
    uint8_t ctrl1;
    if (readRegister(NAU7802_REG_CTRL1, &ctrl1) != NauStatus::OK) {
        return NauStatus::ERR_I2C; // Failed to read CTRL1 register
    }

    // Modify gain bits
    ctrl1 &= 0b11111000; // Clear existing gain
    ctrl1 |= gain; // Set new gain

    // Write back modified CTRL1 register
    if (writeRegister(NAU7802_REG_CTRL1, ctrl1) != NauStatus::OK) {
        return NauStatus::ERR_I2C; // Failed to write CTRL1 register
    }

    // Verify the gain setting
    uint8_t verifyCtrl1;
    if (readRegister(NAU7802_REG_CTRL1, &verifyCtrl1) != NauStatus::OK) {
        return NauStatus::ERR_I2C; // Failed to read back CTRL1 register
    }

    // Check if gain bits match
    if ((verifyCtrl1 & 0b00000111) == gain) {
        return NauStatus::OK; // Success
    }
    else {
        return NauStatus::ERR_I2C; // Verification failed
    }
}

/* -- Private Helpers -- */
NauStatus NAU7802::writeRegister(std::uint8_t reg, std::uint8_t value) {
    if (_i2c->writeByte(_deviceAddress, reg, value)) {
        return NauStatus::OK;
    }
    return NauStatus::ERR_I2C;
}

NauStatus NAU7802::readRegister(std::uint8_t reg, std::uint8_t* value) {
    if (_i2c->readByte(_deviceAddress, reg, value)) {
        return NauStatus::OK;
    }
    return NauStatus::ERR_I2C;
}

NauStatus NAU7802::readRegisters(std::uint8_t reg, std::uint8_t* buffer, std::uint8_t len) {
    if (_i2c->readBytes(_deviceAddress, reg, buffer, len)) {
        return NauStatus::OK;
    }
    return NauStatus::ERR_I2C;
}

NauStatus NAU7802::modifyRegisterBit(std::uint8_t reg, std::uint8_t bitMask, bool state) {
    uint8_t val;
    
    // 1. Check Read Status
    NauStatus status = readRegister(reg, &val);
    if (status != NauStatus::OK) {
        return status; // Propagate error
    }

    // 2. Modify
    if (state) {
        val |= bitMask;
    } else {
        val &= ~bitMask;
    }

    // 3. Return Write Status
    return writeRegister(reg, val);
}

NauStatus NAU7802::calibrate() {
    // TODO: Implement calibration logic based on datasheet
    // Usually involves setting CALS bit in REG0x02
    return modifyRegisterBit(NAU7802_REG_CTRL2, NAU7802_CTRL2_CALS, true); 
}