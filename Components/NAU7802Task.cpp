/*
 * NAU7802Task.cpp
 *
 * Reads the NAU7802 ADC and prints raw samples
 */

#include <NAU7802Task.hpp>
#include <chrono>
#include <cmath>

extern "C" {
#include "main.h"
extern I2C_HandleTypeDef hi2c3;
}

extern "C" void NAU7802Task_HandleDrdyInterrupt(uint16_t gpioPin);

namespace {
constexpr unsigned long NAU7802_SAMPLE_PERIOD_MS = 100;
constexpr unsigned long NAU7802_REINIT_PERIOD_MS = 1000;
constexpr unsigned long NAU7802_COMMAND_TIMEOUT_MS = 20;

// Output unit selection default: false = pounds, true = kilograms.
constexpr bool NAU7802_OUTPUT_IN_KG_DEFAULT = false;

// System-specific scale factors. Tune with a known mass calibration.
constexpr long NAU7802_COUNTS_PER_KG = 100000;

const char* GainToString(uint8_t gain)
{
    switch (gain & 0x07) {
    case NAU7802_GAIN_1X:
        return "1x";
    case NAU7802_GAIN_2X:
        return "2x";
    case NAU7802_GAIN_4X:
        return "4x";
    case NAU7802_GAIN_8X:
        return "8x";
    case NAU7802_GAIN_16X:
        return "16x";
    case NAU7802_GAIN_32X:
        return "32x";
    case NAU7802_GAIN_64X:
        return "64x";
    case NAU7802_GAIN_128X:
        return "128x";
    default:
        return "unknown";
    }
}
}

NAU7802Task::NAU7802Task()
    : Task(TASK1_QUEUE_DEPTH_OBJS),
      _i2cWrapper(&hi2c3),
      _adc(&_i2cWrapper),
      _enableReading(true),
	_enableLogging(false),
	_sensorReady(false),
	_outputInKg(NAU7802_OUTPUT_IN_KG_DEFAULT)
{
    _tareCounts = 0;
    _baselineRunning = false;
    _baselineSamples = 0;
}

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

void NAU7802Task::SetTaskActive(bool enabled)
{
    _enableReading = enabled;

    if (enabled) {
        if (rtTaskHandle != nullptr) {
            vTaskResume(rtTaskHandle);
        }
        SOAR_PRINT("NAU7802Task - NAU7802 read ON\n");
    } else {
        SOAR_PRINT("NAU7802Task - NAU7802 read OFF\n");
        if (rtTaskHandle != nullptr) {
            vTaskSuspend(rtTaskHandle);
        }
    }
}

void NAU7802Task::ToggleTaskActive()
{
    SetTaskActive(!_enableReading);
}

void NAU7802Task::SetLoggingEnabled(bool enabled)
{
    _enableLogging = enabled;
    SOAR_PRINT("NAU7802Task - NAU7802 logging %s\n", _enableLogging ? "ON" : "OFF");
}

void NAU7802Task::PrintStatus()
{
    uint8_t ctrl1 = 0;
    const bool gotCtrl1 = _i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_CTRL1, &ctrl1);
    SOAR_PRINT("NAU7802Task - status read=%s log=%s unit=%s tare=%ld ready=%d drdy=%d\n",
               _enableReading ? "ON" : "OFF",
               _enableLogging ? "ON" : "OFF",
               _outputInKg ? "kg" : "lb",
               _tareCounts,
               _adc.isReady() ? 1 : 0,
               (HAL_GPIO_ReadPin(LC1_DRDY_GPIO_Port, LC1_DRDY_Pin) == GPIO_PIN_SET) ? 1 : 0);
    if (gotCtrl1) {
        SOAR_PRINT("NAU7802Task - status CTRL1=0x%02X gain=%s\n", ctrl1, GainToString(ctrl1));
    }
}

void NAU7802Task::PrintDrdy()
{
    uint8_t ctrl1 = 0;
    const bool pinAsserted = (HAL_GPIO_ReadPin(LC1_DRDY_GPIO_Port, LC1_DRDY_Pin) == GPIO_PIN_SET);
    const bool readyBit = _adc.isReady();
    if (_i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_CTRL1, &ctrl1)) {
        SOAR_PRINT("NAU7802Task - DRDY pin=%d ready=%d CTRL1=0x%02X gain=%s\n",
                   pinAsserted ? 1 : 0,
                   readyBit ? 1 : 0,
                   ctrl1,
                   GainToString(ctrl1));
    } else {
        SOAR_PRINT("NAU7802Task - DRDY pin=%d ready=%d CTRL1=READ_ERR\n",
                   pinAsserted ? 1 : 0,
                   readyBit ? 1 : 0);
    }
}




/**
 * @brief Instance Run loop for NAU7802Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void NAU7802Task::Run(void * pvParams)
{
    (void)pvParams;



    auto initializeSensor = [this]() -> bool {
        if (_adc.begin(NAU7802_GAIN_128X) != NauStatus::OK) {
            SOAR_PRINT("NAU7802Task - NAU7802 init failed, retrying\n");
            return false;
        }

        SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");

        const NauStatus calibrationStatus = _adc.calibrate();
        if (calibrationStatus == NauStatus::OK) {
            SOAR_PRINT("NAU7802Task - NAU7802 calibration complete\n");
        } else {
            SOAR_PRINT("NAU7802Task - NAU7802 calibration failed (%d), continuing\n",
                       static_cast<int>(calibrationStatus));
        }

        return true;
    };

    _sensorReady = initializeSensor();

    unsigned long lastSampleTick = xTaskGetTickCount();
    unsigned long lastInitRetryTick = xTaskGetTickCount();


#if 1
    while (1) {


        const TickType_t now = xTaskGetTickCount();

        // Main read loop: check timing, then check if data is ready, then read
        if (_sensorReady && _enableReading && ((now - lastSampleTick) >= pdMS_TO_TICKS(NAU7802_SAMPLE_PERIOD_MS))) {
            
            // Check if DRDY pin is asserted or device reports ready
            const bool pinSet = (HAL_GPIO_ReadPin(LC1_DRDY_GPIO_Port, LC1_DRDY_Pin) == GPIO_PIN_SET);
            const bool deviceReady = _adc.isReady();

            if (pinSet || deviceReady) {
                lastSampleTick = now;
                NAU7802_OUT adcData{};
                
                if (_adc.readSensor(&adcData) == NauStatus::OK) {
                        if (_enableLogging) {
                            // Print pin and device status
                            SOAR_PRINT("NAU7802Task - pin=%d ready=%d raw: %ld\n",
                                       pinSet ? 1 : 0,
                                       deviceReady ? 1 : 0,
                                       static_cast<long>(adcData.raw_reading));

                            // Read raw bytes and PU_CTRL status for debug
                            uint8_t bytes[3] = {0};
                            uint8_t pu = 0;
                            bool gotBytes = _i2cWrapper.readBytes(NAU7802_I2C_ADDRESS, NAU7802_REG_ADC_B2, bytes, 3);
                            bool gotPu = _i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_PU_CTRL, &pu);

                            const long long milliKg = rawToMilliKg(static_cast<long>(adcData.raw_reading));
                            const long long milliUnits = _outputInKg ? milliKg : milliKgToMilliLb(milliKg);
                            const long wholeUnits = static_cast<long>(milliUnits / 1000LL);
                            long fractionalUnits = static_cast<long>(milliUnits % 1000LL);
                            if (fractionalUnits < 0) {
                                fractionalUnits = -fractionalUnits;
                            }

                            SOAR_PRINT("NAU7802Task - raw: %ld, weight: %ld.%03ld %s\n",
                                       static_cast<long>(adcData.raw_reading),
                                       wholeUnits,
                                       fractionalUnits,
                                       _outputInKg ? "kg" : "lb");

                            if (gotBytes) {
                                SOAR_PRINT("NAU7802Task - raw bytes: 0x%02X 0x%02X 0x%02X\n", bytes[0], bytes[1], bytes[2]);
                            }
                            if (gotPu) {
                                SOAR_PRINT("NAU7802Task - PU_CTRL: 0x%02X\n", pu);
                            }
                            // Also print key control regs
                            uint8_t ctrl1 = 0, ctrl2 = 0, rev = 0;
                            if (_i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_CTRL1, &ctrl1)) {
                                SOAR_PRINT("NAU7802Task - CTRL1: 0x%02X\n", ctrl1);
                            }
                            if (_i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_CTRL2, &ctrl2)) {
                                SOAR_PRINT("NAU7802Task - CTRL2: 0x%02X\n", ctrl2);
                            }
                            if (_i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_REVISION_ID, &rev)) {
                                SOAR_PRINT("NAU7802Task - REV_ID: 0x%02X\n", rev);
                            }
                        }
                    }
                } else if (_enableLogging) {
                    SOAR_PRINT("NAU7802Task - Failed to read sensor data.\n");
                }
            }
        }

        Command cm;
        if (qEvtQueue->Receive(cm, NAU7802_COMMAND_TIMEOUT_MS)) {
            HandleCommand(cm);
        }
    }
#endif

/**
 * @brief HandleCommand handles any command passed to NAU7802Task primary event queue.
 * @param cm Reference to the command object to handle
*/
void NAU7802Task::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND:
    {
        switch (cm.GetTaskCommand()) {
        case NAUTASK_COMMAND_NAU_READ:
        {
            if (_sensorReady) {
                NAU7802_OUT adcData{};
                if (_adc.readSensor(&adcData) == NauStatus::OK) {
                    uint8_t bytes[3] = {0};
                    uint8_t pu = 0;
                    bool gotBytes = _i2cWrapper.readBytes(NAU7802_I2C_ADDRESS, NAU7802_REG_ADC_B2, bytes, 3);
                    bool gotPu = _i2cWrapper.readByte(NAU7802_I2C_ADDRESS, NAU7802_REG_PU_CTRL, &pu);

                    const long long milliKg = rawToMilliKg(static_cast<long>(adcData.raw_reading));
                    const long long milliUnits = _outputInKg ? milliKg : milliKgToMilliLb(milliKg);
                    const long wholeUnits = static_cast<long>(milliUnits / 1000LL);
                    long fractionalUnits = static_cast<long>(milliUnits % 1000LL);
                    if (fractionalUnits < 0) fractionalUnits = -fractionalUnits;

                    if (_enableLogging) {
                        SOAR_PRINT("NAU7802Task - read raw: %ld, weight: %ld.%03ld %s\n",
                                   static_cast<long>(adcData.raw_reading), wholeUnits, fractionalUnits,
                                   _outputInKg ? "kg" : "lb");

                        if (gotBytes) {
                            SOAR_PRINT("NAU7802Task - read raw bytes: 0x%02X 0x%02X 0x%02X\n", bytes[0], bytes[1], bytes[2]);
                        }
                        if (gotPu) {
                            SOAR_PRINT("NAU7802Task - read PU_CTRL: 0x%02X\n", pu);
                        }
                    }
                } else if (_enableLogging) {
                    SOAR_PRINT("NAU7802Task - Read command failed to read sensor data.\n");
                }
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_READ_ISR:
        {
            if (_sensorReady) {
                NAU7802_OUT adcData{};
                (void)_adc.readSensor(&adcData);
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_DRDY:
        {
            PrintDrdy();
            break;
        }
        case NAUTASK_COMMAND_NAU_TOGGLE:
            ToggleTaskActive();
            break;
        case NAUTASK_COMMAND_NAU_ON:
            SetTaskActive(true);
            break;
        case NAUTASK_COMMAND_NAU_OFF:
            SetTaskActive(false);
            break;
        case NAUTASK_COMMAND_NAU_STATUS:
            PrintStatus();
            break;
        case NAUTASK_COMMAND_NAU_ENABLE_LOG:
            SetLoggingEnabled(true);
            break;
        case NAUTASK_COMMAND_NAU_DISABLE_LOG:
            SetLoggingEnabled(false);
            break;
        case NAUTASK_COMMAND_NAU_DUMP_REGS:
        {
            SOAR_PRINT("NAU7802Task - Dumping registers:\n");
            const uint8_t regs[] = {NAU7802_REG_PU_CTRL, NAU7802_REG_CTRL1, NAU7802_REG_CTRL2,
                                     NAU7802_REG_ADC_B2, NAU7802_REG_ADC_B1, NAU7802_REG_ADC_B0,
                                     NAU7802_REG_REVISION_ID};
            for (size_t i = 0; i < sizeof(regs); ++i) {
                uint8_t val = 0;
                if (_i2cWrapper.readByte(NAU7802_I2C_ADDRESS, regs[i], &val)) {
                    SOAR_PRINT("  Reg 0x%02X = 0x%02X\n", regs[i], val);
                } else {
                    SOAR_PRINT("  Reg 0x%02X = READ_ERR\n", regs[i]);
                }
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_TARE:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - Tare: sensor not ready\n");
                break;
            }
            NAU7802_OUT adcData{};
            if (_adc.readSensor(&adcData) == NauStatus::OK) {
                _tareCounts = static_cast<long>(adcData.raw_reading);
                SOAR_PRINT("NAU7802Task - Tare set to %ld\n", _tareCounts);
            } else {
                SOAR_PRINT("NAU7802Task - Failed to perform tare (read error)\n");
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_BASELINE:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - Baseline: sensor not ready\n");
                break;
            }
            const unsigned int N = 64;
            long long sum = 0;
            long long sumsq = 0;
            unsigned int got = 0;
            for (unsigned int i = 0; i < N; ++i) {
                NAU7802_OUT adcData{};
                unsigned int wait = 0;
                while (!_adc.isReady() && wait++ < 200) {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }
                if (_adc.readSensor(&adcData) == NauStatus::OK) {
                    long val = static_cast<long>(adcData.raw_reading);
                    sum += val;
                    sumsq += (long long)val * val;
                    ++got;
                }
            }
            if (got > 0) {
                double mean = (double)sum / got;
                double variance = ((double)sumsq / got) - (mean * mean);
                double stddev = variance > 0 ? sqrt(variance) : 0.0;
                SOAR_PRINT("NAU7802Task - Baseline samples=%u mean=%.0f stddev=%.2f\n", got, mean, stddev);
            } else {
                SOAR_PRINT("NAU7802Task - Baseline: no samples\n");
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_SET_GAIN_1X:
        case NAUTASK_COMMAND_NAU_SET_GAIN_2X:
        case NAUTASK_COMMAND_NAU_SET_GAIN_4X:
        case NAUTASK_COMMAND_NAU_SET_GAIN_8X:
        case NAUTASK_COMMAND_NAU_SET_GAIN_128:
        {
            uint8_t gain = NAU7802_GAIN_1X;
            const char* label = "1x";
            switch (cm.GetTaskCommand()) {
            case NAUTASK_COMMAND_NAU_SET_GAIN_1X:
                gain = NAU7802_GAIN_1X; label = "1x"; break;
            case NAUTASK_COMMAND_NAU_SET_GAIN_2X:
                gain = NAU7802_GAIN_2X; label = "2x"; break;
            case NAUTASK_COMMAND_NAU_SET_GAIN_4X:
                gain = NAU7802_GAIN_4X; label = "4x"; break;
            case NAUTASK_COMMAND_NAU_SET_GAIN_8X:
                gain = NAU7802_GAIN_8X; label = "8x"; break;
            case NAUTASK_COMMAND_NAU_SET_GAIN_128:
                gain = NAU7802_GAIN_128X; label = "128x"; break;
            default:
                break;
            }
            if (_adc.setGain(gain) == NauStatus::OK) {
                SOAR_PRINT("NAU7802Task - Gain set to %s\n", label);
            } else {
                SOAR_PRINT("NAU7802Task - Failed to set gain to %s\n", label);
            }
            break;
        }
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

    cm.Reset();
}

long long NAU7802Task::rawToMilliKg(long rawReading) const {
    if (NAU7802_COUNTS_PER_KG <= 0) {
        return 0;
    }

    return (static_cast<long long>(rawReading) - static_cast<long long>(_tareCounts)) * 1000LL /
           NAU7802_COUNTS_PER_KG;
}

long long NAU7802Task::milliKgToMilliLb(long long milliKg) const {
    return (milliKg * 2204622LL) / 1000000LL;
}

extern "C" void NAU7802Task_HandleDrdyInterrupt(uint16_t gpioPin)
{
    Command cmd(DATA_COMMAND, NAUTASK_COMMAND_NAU_READ_ISR);
    bool shouldSend = false;

    if (gpioPin == LC1_DRDY_Pin || gpioPin == LC2_DRDY_Pin) {
        shouldSend = true;
    }

    if (shouldSend && NAU7802Task::Inst().IsTaskActive()) {
        NAU7802Task::Inst().GetEventQueue()->SendFromISR(cmd);
    }
}
