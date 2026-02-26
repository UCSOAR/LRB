#include "MAX31856MUD+Driver.hpp"
#include <stdint.h>
#include <assert.h>

#define SPI_DUMMY_BYTE 0xFF

//marks driver as initialized
void MAX31856Driver::Init(SPI_HandleTypeDef* hspi_, GPIO_TypeDef* cs_gpio_, uint16_t cs_pin_) {
    hspi = hspi_;
    initialized = true;

    SetCSPin(cs_gpio_, cs_pin_);
    CSHigh();

    // Optionally read CR0 to confirm SPI works
    (void)GetRegister(MAX31856_REG::CR0);
}

uint8_t MAX31856Driver::GetRegister(MAX31856_REG::REG reg) {
    assert(initialized);

    uint8_t tx[2] = {
        uint8_t(reg | 0x80), 
        SPI_DUMMY_BYTE
    };

    uint8_t rx[2] = {0,0};

    CSLow();
    HAL_SPI_TransmitReceive(hspi, tx, rx, 2, 1000);
    CSHigh();

    return rx[1];
}

bool MAX31856Driver::SetRegister(MAX31856_REG::REG reg, uint8_t val) {
    assert(initialized);

    uint8_t tx[2] = {
        uint8_t(reg & 0x7F),
        val
    };

    CSLow();
    HAL_StatusTypeDef r = HAL_SPI_Transmit(hspi, tx, 2, 1000);
    CSHigh();

    return r == HAL_OK;
}

void MAX31856Driver::CSLow() {
    assert(initialized);
    HAL_GPIO_WritePin(cs_gpio, cs_pin, GPIO_PIN_RESET);
}

void MAX31856Driver::CSHigh() {
    assert(initialized);
    HAL_GPIO_WritePin(cs_gpio, cs_pin, GPIO_PIN_SET);
}

void MAX31856Driver::SetCSPin(GPIO_TypeDef* gpio, uint16_t pin) {
    cs_gpio = gpio;
    cs_pin = pin;
}

bool MAX31856Driver::SetMASK(uint8_t value) {
    return SetRegister(MAX31856_REG::MASK, value);
}

uint8_t MAX31856Driver::GetMASK() {
    return GetRegister(MAX31856_REG::MASK);
}

bool MAX31856Driver::SetCJHF(uint8_t value) {
    return SetRegister(MAX31856_REG::CJHF, value);
}

uint8_t MAX31856Driver::GetCJHF() {
    return GetRegister(MAX31856_REG::CJHF);
}

bool MAX31856Driver::SetCR0(uint8_t value) {
    return SetRegister(MAX31856_REG::CR0, value);
}

uint8_t MAX31856Driver::GetCR0() {
    return GetRegister(MAX31856_REG::CR0);
}

bool MAX31856Driver::SetCR1(uint8_t value) {
    return SetRegister(MAX31856_REG::CR1, value);
}

uint8_t MAX31856Driver::GetCR1() {
    return GetRegister(MAX31856_REG::CR1);
}

bool MAX31856Driver::SetCJLF(uint8_t value) {
    return SetRegister(MAX31856_REG::CJLF, value);
}

uint8_t MAX31856Driver::GetCJLF() {
    return GetRegister(MAX31856_REG::CJLF);
}

bool MAX31856Driver::SetLTHFTH(uint8_t value) {
    return SetRegister(MAX31856_REG::LTHFTH, value);
}

uint8_t MAX31856Driver::GetLTHFTH() {
    return GetRegister(MAX31856_REG::LTHFTH);
}

bool MAX31856Driver::SetLTHFTL(uint8_t value) {
    return SetRegister(MAX31856_REG::LTHFTL, value);
}

uint8_t MAX31856Driver::GetLTHFTL() {
    return GetRegister(MAX31856_REG::LTHFTL);
}

bool MAX31856Driver::SetLTLFTH(uint8_t value) {
    return SetRegister(MAX31856_REG::LTLFTH, value);
}

uint8_t MAX31856Driver::GetLTLFTH() {
    return GetRegister(MAX31856_REG::LTLFTH);
}

bool MAX31856Driver::SetCJTO(uint8_t value) {
    return SetRegister(MAX31856_REG::CJTO, value);
}

uint8_t MAX31856Driver::GetCJTO() {
    return GetRegister(MAX31856_REG::CJTO);
}

bool MAX31856Driver::SetCJTH(uint8_t value) {
    return SetRegister(MAX31856_REG::CJTH, value);
}

uint8_t MAX31856Driver::GetCJTH() {
    return GetRegister(MAX31856_REG::CJTH);
}

bool MAX31856Driver::SetCJTL(uint8_t value) {
    return SetRegister(MAX31856_REG::CJTL, value);
}

uint8_t MAX31856Driver::GetCJTL() {
    return GetRegister(MAX31856_REG::CJTL);
}

bool MAX31856Driver::SetLTLFTL(uint8_t value) {
    return SetRegister(MAX31856_REG::LTLFTL, value);
}

uint8_t MAX31856Driver::GetLTLFTL() {
    return GetRegister(MAX31856_REG::LTLFTL);
}

void MAX31856Driver::GetMultipleRegisters(MAX31856_REG::REG startreg, int numBytes, uint8_t* out) {
    assert(initialized);

    uint8_t tx[numBytes + 1];
    uint8_t rx[numBytes + 1];

    tx[0] = startreg | 0x80;  
    for(int i = 1; i <= numBytes; i++) tx[i] = SPI_DUMMY_BYTE;

    CSLow();
    HAL_SPI_TransmitReceive(hspi, tx, rx, numBytes + 1, 1000);
    CSHigh();

    // skip first (command echo)
    for(int i = 0; i < numBytes; i++)
        out[i] = rx[i+1];
}

//thermo couple tempature
float MAX31856Driver::ReadThermocoupleTempC() {
    uint8_t buf[3];
    GetMultipleRegisters(MAX31856_REG::LTCBH, 3, buf);

    // Convert 24-bit 
    int32_t raw = (buf[0] << 16) | (buf[1] << 8) | buf[2];

    // Sign extend
    if(raw & 0x800000)
        raw |= 0xFF000000;

    return raw / 1024.0f;
}

uint8_t MAX31856Driver::GetFaultStatus() {
    return GetRegister(MAX31856_REG::SR);
}
