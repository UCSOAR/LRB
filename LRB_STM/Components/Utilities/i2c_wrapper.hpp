 /**
 ********************************************************************************
 * @file    i2c_wrapper.hpp
 * @author  Javier
 * @date    2025-11-25
 * @brief   I2C communication wrapper.
 ********************************************************************************
 */

#ifndef I2C_WRAPPER_HPP
#define I2C_WRAPPER_HPP


/************************************
 * INCLUDES
 ************************************/
#include <cstdint>

extern "C" {
    #include "main.h"
}



/************************************
 * CLASS DEFINITIONS
 ************************************/
class I2C_Wrapper {
public:
    /**
     * @brief Constructor.
     * @param hi2c Pointer to the HAL I2C handle
     */

    I2C_Wrapper(I2C_HandleTypeDef* hi2c);

    /**
     * @brief Writes a single byte to a specific register.
     * @param devAddr The 8-bit device address (left-shifted)
     * @param regAddr The register address to write to.
     * @param data The byte to write.
     * @return True if successful (HAL_OK), false otherwise.
     */
    bool writeByte(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t data);

    /**
     * @brief Reads a single byte from a specific register.
     * @param devAddr The 8-bit device address (left-shifted)
     * @param regAddr The register address to read from.
     * @return 1 if successful, 0 otherwise.
     */
    bool readByte(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t* dest);

    /**
     * @brief Reads Multiple bytes from a specific register.
     * @param devAddr The 8-bit device address (left-shifted)
     * @param regAddr The register address to read from.
     * @param data Pointer to the buffer to store read data.
     * @param len Number of bytes to read.
     * @return True if successful (HAL_OK), false otherwise.
     */
    bool readBytes(std::uint8_t devAddr, std::uint8_t regAddr, std::uint8_t data[], std::uint8_t len);
private:
    I2C_HandleTypeDef* _hi2c;
};

#endif // I2C_WRAPPER_HPP