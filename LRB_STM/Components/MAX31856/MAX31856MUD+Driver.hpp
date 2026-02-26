#ifndef MAX31856_DRIVER_HPP_
#define MAX31856_DRIVER_HPP_

#include "main.h"
#include <stdint.h>

namespace MAX31856_REG {
    enum REG : uint8_t {
        CR0 = 0x00,
        CR1 = 0x01,
        MASK = 0x02,
        CJHF = 0x03,
        CJLF = 0x04,
        LTHFTH = 0x05,
        LTHFTL = 0x06,
        LTLFTH = 0x07,
        LTLFTL = 0x08,
        CJTO = 0x09,
        CJTH = 0x0A,
        CJTL = 0x0B,
        LTCBH = 0x0C,
        LTCBM = 0x0D,
        LTCBL = 0x0E,
        SR = 0x0F

    };
}

class MAX31856Driver {
public:
    void init(SPI_HandleTypeDef* hspi_, GPIO_TypeDef* cs_gpio_, uint16_t cs_pin_) { Init(hspi_, cs_gpio_, cs_pin_); }
    void task() { 
        /* Periodic logic for thermocouple (to be implemented) */ 
    }
    float ReadThermocoupleTempC();
    void Init(SPI_HandleTypeDef* hspi_, GPIO_TypeDef* cs_gpio_, uint16_t cs_pin_);

    // Add all these missing declarations:
    bool SetMASK(uint8_t value);
    uint8_t GetMASK();
    bool SetCJHF(uint8_t value);
    uint8_t GetCJHF();
    bool SetCR0(uint8_t value);
    uint8_t GetCR0();
    bool SetCR1(uint8_t value);
    uint8_t GetCR1();
    bool SetCJLF(uint8_t value);
    uint8_t GetCJLF();
    bool SetLTHFTH(uint8_t value);
    uint8_t GetLTHFTH();
    bool SetLTHFTL(uint8_t value);
    uint8_t GetLTHFTL();
    bool SetLTLFTH(uint8_t value);
    uint8_t GetLTLFTH();
    bool SetCJTO(uint8_t value);
    uint8_t GetCJTO();
    bool SetCJTH(uint8_t value);
    uint8_t GetCJTH();
    bool SetCJTL(uint8_t value);
    uint8_t GetCJTL();
    bool SetLTLFTL(uint8_t value);
    uint8_t GetLTLFTL();

    void GetMultipleRegisters(MAX31856_REG::REG startreg, int numBytes, uint8_t* out);
    uint8_t GetFaultStatus();

private:
    SPI_HandleTypeDef* hspi;
    GPIO_TypeDef* cs_gpio;
    uint16_t cs_pin;
    bool initialized; // Also make sure this is here!

    void CSLow();
    void CSHigh();
    void SetCSPin(GPIO_TypeDef* gpio, uint16_t pin);
    uint8_t GetRegister(MAX31856_REG::REG reg);
    bool SetRegister(MAX31856_REG::REG reg, uint8_t val);
};

#endif
