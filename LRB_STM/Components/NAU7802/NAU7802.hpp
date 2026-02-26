 /**
 ********************************************************************************
 * @file    NAU7802.hpp
 * @author  Javier
 * @date    2025-11-25
 * @brief   Header for the NAU7802 24-bit ADC Driver.
 ********************************************************************************
 */

#ifndef NAU7802_HPP
#define NAU7802_HPP

/************************************
 * INCLUDES
 ************************************/
#include <cstdint>
#include "i2c_wrapper.hpp"
#include "NAU7802_regs.hpp"

/**
 * @brief Output data structure for the NAU7802.
 */
typedef struct NAU7802_OUTPUT {
    std::int32_t raw_reading;
} NAU7802_OUT;

/************************************
 * CLASS DEFINITIONS
 ************************************/
// New Status Enum
enum class NauStatus : uint8_t {
    OK = 0,
    ERR_I2C,
    ERR_TIMEOUT,
    ERR_NOT_READY,
    ERR_INVALID_ARG
};


class NAU7802 {
public:
    /**
     * @brief Constructs the NAU7802 Driver.
     * @param configs Configuration settings for the sensor.
     * @param i2c_pointer I2C Wrapper for communication.
     */
    NAU7802(I2C_Wrapper* i2c_pointer, std::uint8_t address = NAU7802_I2C_ADDRESS);



    /**
    * @brief Initializes the sensor, resets and waits for power up ready.
    * @return true if sensor detected and powered up, false otherwise.
    */
    NauStatus begin(uint8_t initialGain = NAU7802_GAIN_1X);

    /**
     * @brief Checks the Cycle Ready (CR) bit.
     * @return true if new data is available.
     */
    bool isReady();

    /**
     * @brief Reads the 24-bit signed ADC value.
     * @param dest Pointer to the output struct.
     * @return true if read was successful.
     */
    NauStatus readSensor(NAU7802_OUT *dest);

    /**
     * @brief Performs a software reset of the device.
     */
    NauStatus reset();

    /**
     * @brief Sets the Programmable Gain Amplifier (PGA) setting.
     * @param gain One of NAU7802_GAIN_xxx constants.
     * @return true if write successful.
     */
    NauStatus setGain(std::uint8_t gain);

    /**
     * @brief Calibrates the internal offsets (Optional usage).
     */
    NauStatus calibrate();

private:

    // Private Helpers
    NauStatus writeRegister(std::uint8_t reg, std::uint8_t value);
    NauStatus readRegister(std::uint8_t reg, std::uint8_t* value);
    NauStatus readRegisters(std::uint8_t reg, std::uint8_t* buffer, std::uint8_t len);
    NauStatus modifyRegisterBit(std::uint8_t reg, std::uint8_t bitMask, bool state);

    // Member Variables
    I2C_Wrapper* _i2c;
    std::uint8_t _deviceAddress;

};

#endif /* NAU7802_HPP */