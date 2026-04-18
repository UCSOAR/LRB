/*
 * NAU7802Task.cpp
 *
 * Reads the NAU7802 ADC and prints raw samples
 */

#include <NAU7802Task.hpp>

extern "C" {
#include "main.h"
extern I2C_HandleTypeDef hi2c3;
}

namespace {
constexpr uint32_t NAU7802_SAMPLE_PERIOD_MS = 100;
constexpr uint32_t NAU7802_REINIT_PERIOD_MS = 1000;
constexpr uint32_t NAU7802_COMMAND_TIMEOUT_MS = 20;
}

NAU7802Task::NAU7802Task()
    : Task(TASK1_QUEUE_DEPTH_OBJS),
      _i2cWrapper(&hi2c3),
      _adc(&_i2cWrapper),
      _enableReading(true),
      _enableLogging(true),
      _sensorReady(false)
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
    (void)pvParams;

    _sensorReady = _adc.begin(NAU7802_GAIN_1X) == NauStatus::OK;
    if (_sensorReady) {
        SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");
    } else {
        SOAR_PRINT("NAU7802Task - NAU7802 init failed, retrying\n");
    }

    uint32_t lastSampleTick = xTaskGetTickCount();
    uint32_t lastInitRetryTick = xTaskGetTickCount();

    while (1) {
        const uint32_t now = xTaskGetTickCount();

        if (!_sensorReady && ((now - lastInitRetryTick) >= pdMS_TO_TICKS(NAU7802_REINIT_PERIOD_MS))) {
            lastInitRetryTick = now;
            _sensorReady = _adc.begin(NAU7802_GAIN_1X) == NauStatus::OK;
            if (_sensorReady) {
                SOAR_PRINT("NAU7802Task - NAU7802 initialized\n");
            }
        }

        if (_sensorReady && _enableReading && ((now - lastSampleTick) >= pdMS_TO_TICKS(NAU7802_SAMPLE_PERIOD_MS))) {
            lastSampleTick = now;

            if (_adc.isReady()) {
                NAU7802_OUT adcData{};
                if (_adc.readSensor(&adcData) == NauStatus::OK) {
                    if (_enableLogging) {
                        SOAR_PRINT("NAU7802Task - NAU7802 raw: %ld\n", static_cast<long>(adcData.raw_reading));
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
            _enableReading = !_enableReading;
            SOAR_PRINT("NAU7802Task - NAU7802 read %s\n", _enableReading ? "ON" : "OFF");
            break;
        case TASK1_COMMAND_NAU_ON:
            _enableReading = true;
            SOAR_PRINT("NAU7802Task - NAU7802 read ON\n");
            break;
        case TASK1_COMMAND_NAU_OFF:
            _enableReading = false;
            SOAR_PRINT("NAU7802Task - NAU7802 read OFF\n");
            break;
        case TASK1_COMMAND_NAU_STATUS:
            SOAR_PRINT("NAU7802Task - NAU7802 read status: %s\n", _enableReading ? "ON" : "OFF");
            break;
        case TASK1_COMMAND_NAU_ENABLE_LOG:
            _enableLogging = true;
            SOAR_PRINT("NAU7802Task - NAU7802 logging ON\n");
            break;
        case TASK1_COMMAND_NAU_DISABLE_LOG:
            _enableLogging = false;
            SOAR_PRINT("NAU7802Task - NAU7802 logging OFF\n");
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

    cm.Reset();
}