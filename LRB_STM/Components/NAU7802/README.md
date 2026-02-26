
# STM32 C++ Driver for NAU7802

This is a C++ driver for the Nuvoton NAU7802 24-bit ADC, designed to be used with the STM32 HAL library in a C++ environment

##

### Features

- Handles sensor initialization and power-up sequence
- Software reset
- Sets PGA gain (from 1x to 128x)
- Checks if new conversion data is ready
- Reads the 24-bit signed ADC result

### Project Structure

The driver is split into logical components to make it easy to understand and port:

- `i2c_wrapper.hpp` / `.cpp`
  - A small C++ class that wraps the C-style STM32 HAL I2C functions (`HAL_I2C_Mem_Write` / `HAL_I2C_Mem_Read`).
  - Acts as the low-level communication handler for register reads/writes.
- `NAU7802_regs.hpp`
  - Register addresses, bit masks, and gain values for the NAU7802 (datasheet-based definitions).
- `NAU7802.hpp` / `.cpp`
  - The main driver with logic for communicating with the sensor.
  - Defines `NAU7802_OUT` for output data.
  - Uses `I2C_Wrapper` to perform all register operations.
- `main_read_test.cpp` / `ExampleUsage.cpp`
  - Example `main()` files demonstrating driver initialization and reading.
- `NAU7802Task.hpp` / `.cpp`
  - Initializes Wrapper and Tasks and handles task specific commands

### Complete example

See `main_read_test.cpp` in this driver folder for a complete example that initializes the sensor at 1x gain and continuously reads raw ADC values.

### Dependencies

- STM32 HAL (driver uses HAL I2C functions)
- C++11 or newer

### Datasheet

For full register details and calibration notes, consult the NAU7802 datasheet (e.g. en-us--DS_NAU7802_DataSheet_EN_Rev2.6.pdf).
