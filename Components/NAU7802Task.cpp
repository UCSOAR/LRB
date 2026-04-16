/*
 * NAU7802Task.cpp
 *
 * Reads the NAU7802 ADC and prints raw samples
 */

#include <NAU7802Task.hpp>
#include "main.h"

extern "C" {
extern I2C_HandleTypeDef hi2c3;
}

namespace {
constexpr uint16_t NAU7802_I2C_ADDR = (0x2AU << 1);
constexpr uint8_t NAU7802_REG_PU_CTRL = 0x00;
constexpr uint8_t NAU7802_REG_CTRL1 = 0x01;
constexpr uint8_t NAU7802_REG_ADC_B2 = 0x12;
constexpr uint8_t NAU7802_PU_CTRL_RR = (1U << 0);
constexpr uint8_t NAU7802_PU_CTRL_PUD = (1U << 1);
constexpr uint8_t NAU7802_PU_CTRL_PUA = (1U << 2);
constexpr uint8_t NAU7802_PU_CTRL_PUR = (1U << 3);
constexpr uint8_t NAU7802_PU_CTRL_CR = (1U << 5);
constexpr uint8_t NAU7802_GAIN_1X = 0x00;

bool NauWriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c3, NAU7802_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 50) == HAL_OK;
}

bool NauReadReg(uint8_t reg, uint8_t* value)
{
    return HAL_I2C_Mem_Read(&hi2c3, NAU7802_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, value, 1, 50) == HAL_OK;
}

bool NauReadRegs(uint8_t reg, uint8_t* data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c3, NAU7802_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 50) == HAL_OK;
}

bool NauModifyRegBit(uint8_t reg, uint8_t bitMask, bool state)
{
    uint8_t val = 0;
    if (!NauReadReg(reg, &val)) {
        return false;
    }

    if (state) {
        val |= bitMask;
    } else {
        val &= static_cast<uint8_t>(~bitMask);
    }
    return NauWriteReg(reg, val);
}

bool NauInit()
{
    // Software reset sequence.
    if (!NauModifyRegBit(NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_RR, true)) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    if (!NauModifyRegBit(NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_RR, false)) {
        return false;
    }

    // Power-up analog + digital sections.
    const uint8_t powerUp = static_cast<uint8_t>(NAU7802_PU_CTRL_PUA | NAU7802_PU_CTRL_PUD);
    if (!NauWriteReg(NAU7802_REG_PU_CTRL, powerUp)) {
        return false;
    }

    // Wait for power-up ready bit.
    for (uint8_t attempt = 0; attempt < 100; ++attempt) {
        uint8_t puCtrl = 0;
        if (NauReadReg(NAU7802_REG_PU_CTRL, &puCtrl) && ((puCtrl & NAU7802_PU_CTRL_PUR) != 0U)) {
            return NauWriteReg(NAU7802_REG_CTRL1, NAU7802_GAIN_1X);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return false;
}

bool NauReadRaw(int32_t* outRaw)
{
    uint8_t puCtrl = 0;
    if (!NauReadReg(NAU7802_REG_PU_CTRL, &puCtrl)) {
        return false;
    }
    if ((puCtrl & NAU7802_PU_CTRL_CR) == 0U) {
        return false;
    }

    uint8_t rawBytes[3] = {0};
    if (!NauReadRegs(NAU7802_REG_ADC_B2, rawBytes, 3)) {
        return false;
    }

    int32_t value = (static_cast<int32_t>(rawBytes[0]) << 16) |
                    (static_cast<int32_t>(rawBytes[1]) << 8) |
                    static_cast<int32_t>(rawBytes[2]);
    if ((value & 0x00800000) != 0) {
        value |= static_cast<int32_t>(0xFF000000);
    }

    *outRaw = value;
    return true;
}
} // namespace

/**
 * @brief Initializes NAU7802Task with the RTOS scheduler
*/
void NAU7802Task::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize NAU7802Task twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)NAU7802Task::RunTask,
            (const char*)"NAU7802Task",
            (uint16_t)TASK1_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK1_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "NAU7802Task::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance Run loop for NAU7802Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void NAU7802Task::Run(void * pvParams)
{
    bool nauInitialized = NauInit();
    if (nauInitialized) {
        SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");
    } else {
        SOAR_PRINT("NAU7802Task - NAU7802 init failed, retrying\n");
    }

    uint32_t lastSampleTick = xTaskGetTickCount();
    uint32_t lastInitRetryTick = xTaskGetTickCount();

    while(1) {
        const uint32_t now = xTaskGetTickCount();
        if (!nauInitialized && (now - lastInitRetryTick) >= pdMS_TO_TICKS(1000)) {
            lastInitRetryTick = now;
            nauInitialized = NauInit();
            if (nauInitialized) {
                SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");
            }
        }

        if (nauInitialized && nauReadEnabled_ && (now - lastSampleTick) >= pdMS_TO_TICKS(100)) {
            lastSampleTick = now;
            int32_t rawReading = 0;
            if (NauReadRaw(&rawReading)) {
                SOAR_PRINT("NAU7802Task - NAU7802 raw: %ld\n", static_cast<long>(rawReading));
            }
        }

        Command cm;
        if (qEvtQueue->Receive(cm, 20)) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to NAU7802Task primary event queue.
 * @param cm Reference to the command object to handle
*/
void NAU7802Task::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        case TASK1_COMMAND_NAU_TOGGLE:
            nauReadEnabled_ = !nauReadEnabled_;
            SOAR_PRINT("NAU7802Task - NAU7802 read %s\n", nauReadEnabled_ ? "ON" : "OFF");
            break;
        case TASK1_COMMAND_NAU_ON:
            nauReadEnabled_ = true;
            SOAR_PRINT("NAU7802Task - NAU7802 read ON\n");
            break;
        case TASK1_COMMAND_NAU_OFF:
            nauReadEnabled_ = false;
            SOAR_PRINT("NAU7802Task - NAU7802 read OFF\n");
            break;
        case TASK1_COMMAND_NAU_STATUS:
            SOAR_PRINT("NAU7802Task - NAU7802 read status: %s\n", nauReadEnabled_ ? "ON" : "OFF");
            break;
        default:
            SOAR_PRINT("NAU7802Task - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("NAU7802Task - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    // Reset allocated data
    cm.Reset();
}
