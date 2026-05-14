/*
 * NAU7802Task.cpp
 *
 * Polls the NAU7802 ADC and prints raw samples when logging is enabled.
 */

#include <NAU7802Task.hpp>

extern "C" {
#include "main.h"
extern I2C_HandleTypeDef hi2c3;
}

extern "C" void NAU7802Task_HandleDrdyInterrupt(uint16_t gpioPin);

namespace {
constexpr unsigned long NAU7802_SAMPLE_PERIOD_MS = 100;
constexpr unsigned long NAU7802_REINIT_PERIOD_MS = 1000;
constexpr unsigned long NAU7802_COMMAND_TIMEOUT_MS = 20;
constexpr float NAU7802_EMA_ALPHA_DEFAULT = 0.1f;
constexpr uint8_t NAU7802_CAL_SAMPLES = 8;
constexpr float NAU7802_LB_PER_G = 0.00220462262f;
constexpr int32_t NAU7802_STABILITY_COUNTS = 5000;

const char* LdoToString(NAU7802_LDOVoltage ldo)
{
    switch (ldo) {
    case NAU7802_4V5:
        return "4.5V";
    case NAU7802_4V2:
        return "4.2V";
    case NAU7802_3V9:
        return "3.9V";
    case NAU7802_3V6:
        return "3.6V";
    case NAU7802_3V3:
        return "3.3V";
    case NAU7802_3V0:
        return "3.0V";
    case NAU7802_2V7:
        return "2.7V";
    case NAU7802_2V4:
        return "2.4V";
    case NAU7802_EXTERNAL:
        return "External";
    default:
        return "Unknown";
    }
}

const char* GainToString(NAU7802_Gain gain)
{
    switch (gain) {
    case NAU7802_GAIN_1:
        return "1x";
    case NAU7802_GAIN_2:
        return "2x";
    case NAU7802_GAIN_4:
        return "4x";
    case NAU7802_GAIN_8:
        return "8x";
    case NAU7802_GAIN_16:
        return "16x";
    case NAU7802_GAIN_32:
        return "32x";
    case NAU7802_GAIN_64:
        return "64x";
    case NAU7802_GAIN_128:
        return "128x";
    default:
        return "Unknown";
    }
}

const char* RateToString(NAU7802_SampleRate rate)
{
    switch (rate) {
    case NAU7802_RATE_10SPS:
        return "10 SPS";
    case NAU7802_RATE_20SPS:
        return "20 SPS";
    case NAU7802_RATE_40SPS:
        return "40 SPS";
    case NAU7802_RATE_80SPS:
        return "80 SPS";
    case NAU7802_RATE_320SPS:
        return "320 SPS";
    default:
        return "Unknown";
    }
}

void UpdateEma(float alpha, int32_t raw, float *ema, bool *initialized)
{
    if (ema == nullptr || initialized == nullptr) {
        return;
    }

    const float rawF = static_cast<float>(raw);
    if (!*initialized) {
        *ema = rawF;
        *initialized = true;
        return;
    }

    *ema = (alpha * rawF) + ((1.0f - alpha) * (*ema));
}

bool ReadStableAverage(Adafruit_NAU7802 &adc, uint8_t samples, int32_t *outRaw,
                       int32_t *outSpread)
{
    if (outRaw == nullptr || samples == 0) {
        return false;
    }

    long long sum = 0;
    int32_t minVal = 0;
    int32_t maxVal = 0;
    for (uint8_t i = 0; i < samples; ++i) {
        while (!adc.available()) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        const int32_t sample = adc.read();
        if (i == 0) {
            minVal = sample;
            maxVal = sample;
        } else {
            if (sample < minVal) {
                minVal = sample;
            }
            if (sample > maxVal) {
                maxVal = sample;
            }
        }
        sum += static_cast<long long>(sample);
    }

    const int32_t spread = maxVal - minVal;
    if (outSpread != nullptr) {
        *outSpread = spread;
    }
    if (spread > NAU7802_STABILITY_COUNTS) {
        return false;
    }

    *outRaw = static_cast<int32_t>(sum / samples);
    return true;
}

float ComputeWeightGrams(float slopeGPerCount, float offsetG, float emaValue)
{
    return (slopeGPerCount * emaValue) + offsetG;
}
}

NAU7802Task::NAU7802Task()
    : Task(NAU_TASK_QUEUE_DEPTH_OBJS),
      _adc(&hi2c3),
      _enableReading(true),
      _enableLogging(false),
    _sensorReady(false),
    _emaAlpha(NAU7802_EMA_ALPHA_DEFAULT),
    _emaValue(0.0f),
    _emaInitialized(false),
    _calibHave500g(false),
    _calibHave1000g(false),
    _calibRaw500g(0),
    _calibRaw1000g(0),
    _calibSlopeGPerCount(0.0f),
    _calibOffsetG(0.0f),
    _calibValid(false),
    _tareGrams(0.0f),
    _tareValid(false)
{
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
            (uint16_t)NAU_TASK_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)NAU_TASK_RTOS_PRIORITY,
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
    const NAU7802_Gain gain = _adc.getGain();
    const NAU7802_SampleRate rate = _adc.getRate();
    const NAU7802_LDOVoltage ldo = _adc.getLDO();
    const bool available = _adc.available();

    SOAR_PRINT("NAU7802Task - status read=%s log=%s ready=%d avail=%d cal=%s tare=%s alpha=%.2f gain=%s rate=%s ldo=%s\n",
               _enableReading ? "ON" : "OFF",
               _enableLogging ? "ON" : "OFF",
               _sensorReady ? 1 : 0,
               available ? 1 : 0,
               _calibValid ? "ON" : "OFF",
               _tareValid ? "ON" : "OFF",
               static_cast<double>(_emaAlpha),
               GainToString(gain),
               RateToString(rate),
               LdoToString(ldo));
}

void NAU7802Task::PrintDrdy()
{
    SOAR_PRINT("NAU7802Task - DRDY avail=%d\n", _adc.available() ? 1 : 0);
}

/**
 * @brief Instance Run loop for NAU7802Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
 */
void NAU7802Task::Run(void * pvParams)
{
    (void)pvParams;

    auto initializeSensor = [this]() -> bool {
        if (!_adc.begin(&hi2c3)) {
            SOAR_PRINT("NAU7802Task - NAU7802 init failed, retrying\n");
            return false;
        }

        SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");

        if (!_adc.setLDO(NAU7802_4V5)) {
            SOAR_PRINT("NAU7802Task - Failed to set LDO\n");
            return false;
        }

        if (!_adc.setGain(NAU7802_GAIN_128)) {
            SOAR_PRINT("NAU7802Task - Failed to set gain\n");
            return false;
        }

        if (!_adc.setRate(NAU7802_RATE_10SPS)) {
            SOAR_PRINT("NAU7802Task - Failed to set rate\n");
            return false;
        }

        if (!_adc.setChannel(0)) {
            SOAR_PRINT("NAU7802Task - Failed to set channel\n");
            return false;
        }

        if (!_adc.setPGACap(true)) {
            SOAR_PRINT("NAU7802Task - Failed to set PGA cap\n");
            return false;
        }

        SOAR_PRINT("NAU7802Task - LDO %s, gain %s, rate %s\n",
                   LdoToString(_adc.getLDO()),
                   GainToString(_adc.getGain()),
                   RateToString(_adc.getRate()));

        if (!_adc.calibrate(NAU7802_CALMOD_INTERNAL)) {
            SOAR_PRINT("NAU7802Task - NAU7802 calibration failed, continuing\n");
        }

        for (uint8_t i = 0; i < 10; ++i) {
            while (!_adc.available()) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            (void)_adc.read();
        }

        return true;
    };

    _sensorReady = initializeSensor();

    unsigned long lastSampleTick = xTaskGetTickCount();
    unsigned long lastInitRetryTick = xTaskGetTickCount();

    while (1) {
        const TickType_t now = xTaskGetTickCount();

        if (!_sensorReady && ((now - lastInitRetryTick) >= pdMS_TO_TICKS(NAU7802_REINIT_PERIOD_MS))) {
            lastInitRetryTick = now;
            _sensorReady = initializeSensor();
        }

        if (_sensorReady && _enableReading && ((now - lastSampleTick) >= pdMS_TO_TICKS(NAU7802_SAMPLE_PERIOD_MS))) {
            lastSampleTick = now;
            while (!_adc.available()) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            const int32_t raw = _adc.read();
            UpdateEma(_emaAlpha, raw, &_emaValue, &_emaInitialized);
            if (_enableLogging) {
                if (_calibValid) {
                    float grams = ComputeWeightGrams(_calibSlopeGPerCount, _calibOffsetG, _emaValue);
                    if (_tareValid) {
                        grams -= _tareGrams;
                    }
                    const float kg = grams / 1000.0f;
                    const float lb = grams * NAU7802_LB_PER_G;
                    SOAR_PRINT("NAU7802Task - raw: %ld ema: %.1f weight: %.3f kg (%.3f lb)\n",
                               static_cast<long>(raw),
                               static_cast<double>(_emaValue),
                               static_cast<double>(kg),
                               static_cast<double>(lb));
                } else {
                    SOAR_PRINT("NAU7802Task - raw: %ld ema: %.1f\n",
                               static_cast<long>(raw),
                               static_cast<double>(_emaValue));
                }
            }
        }

        Command cm;
        if (qEvtQueue->Receive(cm, NAU7802_COMMAND_TIMEOUT_MS)) {
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
    auto finalizeCalibration = [this]() {
        const int32_t delta = _calibRaw1000g - _calibRaw500g;
        if (delta == 0) {
            _calibValid = false;
            SOAR_PRINT("NAU7802Task - Calibration failed (no delta)\n");
            return;
        }

        _calibSlopeGPerCount = 500.0f / static_cast<float>(delta);
        _calibOffsetG = 500.0f - (_calibSlopeGPerCount * static_cast<float>(_calibRaw500g));
        _calibValid = true;
        _tareGrams = 0.0f;
        _tareValid = false;
        SOAR_PRINT("NAU7802Task - Calibration set: slope=%.6f g/count offset=%.3f g\n",
                   static_cast<double>(_calibSlopeGPerCount),
                   static_cast<double>(_calibOffsetG));
    };

    switch (cm.GetCommand()) {
    case DATA_COMMAND:
    {
        switch (cm.GetTaskCommand()) {
        case NAUTASK_COMMAND_NAU_READ:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - read skipped (not ready)\n");
                break;
            }
            while (!_adc.available()) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            const int32_t raw = _adc.read();
            UpdateEma(_emaAlpha, raw, &_emaValue, &_emaInitialized);
            if (_calibValid) {
                float grams = ComputeWeightGrams(_calibSlopeGPerCount, _calibOffsetG, _emaValue);
                if (_tareValid) {
                    grams -= _tareGrams;
                }
                const float kg = grams / 1000.0f;
                const float lb = grams * NAU7802_LB_PER_G;
                SOAR_PRINT("NAU7802Task - read raw: %ld ema: %.1f weight: %.3f kg (%.3f lb)\n",
                           static_cast<long>(raw),
                           static_cast<double>(_emaValue),
                           static_cast<double>(kg),
                           static_cast<double>(lb));
            } else {
                SOAR_PRINT("NAU7802Task - read raw: %ld ema: %.1f\n",
                           static_cast<long>(raw),
                           static_cast<double>(_emaValue));
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_READ_ISR:
            if (_sensorReady && _adc.available()) {
                (void)_adc.read();
            }
            break;
        case NAUTASK_COMMAND_NAU_DRDY:
            PrintDrdy();
            break;
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
        case NAUTASK_COMMAND_NAU_SET_GAIN_1X:
            _adc.setGain(NAU7802_GAIN_1);
            SOAR_PRINT("NAU7802Task - Gain set to 1x\n");
            break;
        case NAUTASK_COMMAND_NAU_SET_GAIN_2X:
            _adc.setGain(NAU7802_GAIN_2);
            SOAR_PRINT("NAU7802Task - Gain set to 2x\n");
            break;
        case NAUTASK_COMMAND_NAU_SET_GAIN_4X:
            _adc.setGain(NAU7802_GAIN_4);
            SOAR_PRINT("NAU7802Task - Gain set to 4x\n");
            break;
        case NAUTASK_COMMAND_NAU_SET_GAIN_8X:
            _adc.setGain(NAU7802_GAIN_8);
            SOAR_PRINT("NAU7802Task - Gain set to 8x\n");
            break;
        case NAUTASK_COMMAND_NAU_SET_GAIN_128:
            _adc.setGain(NAU7802_GAIN_128);
            SOAR_PRINT("NAU7802Task - Gain set to 128x\n");
            break;
        case NAUTASK_COMMAND_NAU_TARE:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - Tare skipped (not ready)\n");
                break;
            }
            if (!_calibValid) {
                SOAR_PRINT("NAU7802Task - Tare skipped (calibration required)\n");
                break;
            }
            int32_t raw = 0;
            int32_t spread = 0;
            if (!ReadStableAverage(_adc, NAU7802_CAL_SAMPLES, &raw, &spread)) {
                SOAR_PRINT("NAU7802Task - Tare failed (unstable, spread=%ld)\n",
                           static_cast<long>(spread));
                break;
            }
            UpdateEma(_emaAlpha, raw, &_emaValue, &_emaInitialized);
            _tareGrams = ComputeWeightGrams(_calibSlopeGPerCount, _calibOffsetG, _emaValue);
            _tareValid = true;
            SOAR_PRINT("NAU7802Task - Tare set to %.3f g\n",
                       static_cast<double>(_tareGrams));
            break;
        }
        case NAUTASK_COMMAND_NAU_CAL_500G:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - Calibration skipped (not ready)\n");
                break;
            }
            int32_t raw = 0;
            int32_t spread = 0;
            if (!ReadStableAverage(_adc, NAU7802_CAL_SAMPLES, &raw, &spread)) {
                SOAR_PRINT("NAU7802Task - Calibration failed (unstable, spread=%ld)\n",
                           static_cast<long>(spread));
                break;
            }
            _calibRaw500g = raw;
            _calibHave500g = true;
            SOAR_PRINT("NAU7802Task - Calibration 500g raw=%ld\n", static_cast<long>(raw));
            if (_calibHave1000g) {
                finalizeCalibration();
            }
            break;
        }
        case NAUTASK_COMMAND_NAU_CAL_1000G:
        {
            if (!_sensorReady) {
                SOAR_PRINT("NAU7802Task - Calibration skipped (not ready)\n");
                break;
            }
            int32_t raw = 0;
            int32_t spread = 0;
            if (!ReadStableAverage(_adc, NAU7802_CAL_SAMPLES, &raw, &spread)) {
                SOAR_PRINT("NAU7802Task - Calibration failed (unstable, spread=%ld)\n",
                           static_cast<long>(spread));
                break;
            }
            _calibRaw1000g = raw;
            _calibHave1000g = true;
            SOAR_PRINT("NAU7802Task - Calibration 1000g raw=%ld\n", static_cast<long>(raw));
            if (_calibHave500g) {
                finalizeCalibration();
            }
            break;
        }
        default:
            SOAR_PRINT("NAU7802Task - Command not supported (%d)\n", cm.GetTaskCommand());
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
