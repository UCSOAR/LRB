/*
 * MAX31856Task.cpp
 *
 * Reads MAX31856 thermocouple samples and prints temperature.
 */

#include <MAX31856Task.hpp>
#include "../SoarDrivers/MAX31856MUD+/MAX31856MUD+Driver.hpp"

extern "C" {
#include "main.h"
extern SPI_HandleTypeDef hspi1;
}

namespace {
constexpr int MAX31856_SENSOR_COUNT = MAX31856Task::NUM_SENSORS;
constexpr int MAX31856_ACTIVE_SENSORS = 2; // TODO:  Debug: only TC1 is active.
MAX31856Driver gMax31856Drivers[MAX31856_SENSOR_COUNT];

constexpr unsigned long MAX31856_REINIT_PERIOD_MS = 1000;
constexpr unsigned long MAX31856_COMMAND_TIMEOUT_MS = 20;

constexpr uint8_t MAX31856_CR0_INIT =
		MAX31856_REG::CR0_CJ
		| MAX31856_REG::CR0_FAULT;

constexpr uint8_t MAX31856_CR1_INIT =
    MAX31856_REG::CR1_TYPE_K
    | MAX31856_REG::CR1_AVG_4;

constexpr uint8_t MAX31856_MASK_INIT = 0x00; // TODO: MASK LATER -- Do not mask faults during debug.

constexpr int MAX31856_DEBUG_SAMPLES = 1;
constexpr uint32_t MAX31856_DEBUG_SAMPLE_DELAY_MS = 10;

constexpr bool MAX31856_OUTPUT_IN_C_DEFAULT = true;

GPIO_TypeDef* const MAX31856_CS_GPIO_PORTS[MAX31856_SENSOR_COUNT] = {
    TC1_cs_GPIO_Port,
    TC2_cs_GPIO_Port,
    TC3_cs_GPIO_Port
	// ADD more ports if you need
};

const uint16_t MAX31856_CS_PINS[MAX31856_SENSOR_COUNT] = {
    static_cast<uint16_t>(TC1_cs_Pin),
    static_cast<uint16_t>(TC2_cs_Pin),
    static_cast<uint16_t>(TC3_cs_Pin)
};

GPIO_TypeDef* const MAX31856_READY_GPIO_PORTS[MAX31856_SENSOR_COUNT] = {
    TC1_nReady_GPIO_Port,
    TC2_nReady_GPIO_Port,
    TC3_nReady_GPIO_Port
};

const uint16_t MAX31856_READY_PINS[MAX31856_SENSOR_COUNT] = {
    static_cast<uint16_t>(TC1_nReady_Pin),
    static_cast<uint16_t>(TC2_nReady_Pin),
    static_cast<uint16_t>(TC3_nReady_Pin)
};

volatile uint32_t gMax31856DrdyCounts[MAX31856_SENSOR_COUNT] = {0};

float celsiusToFahrenheit(float tempC) {
    return (tempC * 9.0f / 5.0f) + 32.0f;
}
}

MAX31856Task::MAX31856Task()
    : Task(MAX_TASK_QUEUE_DEPTH_OBJS),
            _enableLogging(true),
            _outputInC(MAX31856_OUTPUT_IN_C_DEFAULT)
{
        for (int i = 0; i < NUM_SENSORS; ++i) {
                _thermos[i] = &gMax31856Drivers[i];
                _enableReading[i] = true;
                _sensorReady[i] = false;
        }
}

bool SendMax31856TaskCommand(uint16_t taskCommand)
{
    Command cmd(DATA_COMMAND, taskCommand);
    return MAX31856Task::Inst().GetEventQueue()->Send(cmd);
}

/**
 * @brief Initializes MAX31856Task with the RTOS scheduler
 */
void MAX31856Task::InitTask()
{
    // asser 1 init only
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize MAX31856Task twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)MAX31856Task::RunTask,
            (const char*)"MAX31856Task",
            (uint16_t)MAX_TASK_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)MAX_TASK_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation
    SOAR_ASSERT(rtValue == pdPASS, "MAX31856Task::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance run loop for MAX31856Task.
 * @param pvParams RTOS passed void parameters, unused.
 */
void MAX31856Task::Run(void* pvParams)
{
    (void)pvParams;

    for (int i = 0; i < MAX31856Task::NUM_SENSORS; ++i) {
        HAL_GPIO_WritePin(MAX31856_CS_GPIO_PORTS[i], MAX31856_CS_PINS[i], GPIO_PIN_SET);
    }

    auto initializeSensor = [this](int index) -> bool {
        MAX31856Driver* thermo = _thermos[index];
        thermo->Init(&hspi1, MAX31856_CS_GPIO_PORTS[index], MAX31856_CS_PINS[index]);
        const bool cr0Ok = thermo->SetCR0(MAX31856_REG::CR0_CONV_MODE);
        const bool cr1Ok = thermo->SetCR1(MAX31856_CR1_INIT);
        const bool maskOk = thermo->SetMASK(MAX31856_MASK_INIT);
        if (!cr0Ok || !cr1Ok || !maskOk) {
            SOAR_PRINT("MAX31856Task - TC%d init failed (CR0:%d CR1:%d MASK:%d), retrying\n",
                       index + 1,
                       cr0Ok ? 1 : 0,
                       cr1Ok ? 1 : 0,
                       maskOk ? 1 : 0);
            return false;
        }

        SOAR_PRINT("MAX31856Task - TC%d initialized\n", index + 1);
        SOAR_PRINT("MAX31856Task - TC%d CR0=0x%02X CR1=0x%02X MASK=0x%02X SR=0x%02X\n",
                   index + 1,
                   static_cast<unsigned int>(thermo->GetCR0()),
                   static_cast<unsigned int>(thermo->GetCR1()),
                   static_cast<unsigned int>(thermo->GetMASK()),
                   static_cast<unsigned int>(thermo->GetFaultStatus()));
                   
        return true;
    };

    for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
        _sensorReady[i] = initializeSensor(i);
        if (_sensorReady[i]
            && (HAL_GPIO_ReadPin(MAX31856_READY_GPIO_PORTS[i], MAX31856_READY_PINS[i]) == GPIO_PIN_RESET)) {
            Command cmd(DATA_COMMAND, MAX31856_TASK_COMMAND_READ_TC1 + i);
            HandleCommand(cmd);
        }
    }

    unsigned long lastInitRetryTick[NUM_SENSORS];
    const unsigned long initStartTick = xTaskGetTickCount();
    for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
        lastInitRetryTick[i] = initStartTick;
    }

    while (1) {
        const unsigned long now = xTaskGetTickCount();

        for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
            if (!_sensorReady[i]
                && ((now - lastInitRetryTick[i]) >= pdMS_TO_TICKS(MAX31856_REINIT_PERIOD_MS))) {
                lastInitRetryTick[i] = now;
                _sensorReady[i] = initializeSensor(i);
                if (_sensorReady[i]
                    && (HAL_GPIO_ReadPin(MAX31856_READY_GPIO_PORTS[i], MAX31856_READY_PINS[i]) == GPIO_PIN_RESET)) {
                    Command cmd(DATA_COMMAND, MAX31856_TASK_COMMAND_READ_TC1 + i);
                    HandleCommand(cmd);
                }
            }
        }

        Command cm;
        if (qEvtQueue->Receive(cm, MAX31856_COMMAND_TIMEOUT_MS)) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to MAX31856Task primary event queue.
 * @param cm Reference to the command object to handle
 */
void MAX31856Task::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        case MAX31856_TASK_COMMAND_TOGGLE:
            for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
                _enableReading[i] = !_enableReading[i];
            }
            SOAR_PRINT("MAX31856Task - MAX31856 read toggled for all sensors\n");
            break;
        case MAX31856_TASK_COMMAND_ON:
            for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
                _enableReading[i] = true;
            }
            SOAR_PRINT("MAX31856Task - MAX31856 read ON (all)\n");
            break;
        case MAX31856_TASK_COMMAND_OFF:
            for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
                _enableReading[i] = false;
            }
            SOAR_PRINT("MAX31856Task - MAX31856 read OFF (all)\n");
            break;
        case MAX31856_TASK_COMMAND_STATUS:
            for (int i = 0; i < MAX31856_ACTIVE_SENSORS; ++i) {
                const int drdyState = (HAL_GPIO_ReadPin(MAX31856_READY_GPIO_PORTS[i], MAX31856_READY_PINS[i]) == GPIO_PIN_RESET) ? 0 : 1;
                SOAR_PRINT("MAX31856Task - TC%d read %s, nReady=%d, drdyCount=%lu\n",
                           i + 1,
                           _enableReading[i] ? "ON" : "OFF",
                           drdyState,
                           static_cast<unsigned long>(gMax31856DrdyCounts[i]));
            }
            SOAR_PRINT("MAX31856Task - MAX31856 unit: %s\n", _outputInC ? "C" : "F");
            break;
        case MAX31856_TASK_COMMAND_ENABLE_LOG:
            _enableLogging = true;
            SOAR_PRINT("MAX31856Task - MAX31856 logging ON\n");
            break;
        case MAX31856_TASK_COMMAND_DISABLE_LOG:
            _enableLogging = false;
            SOAR_PRINT("MAX31856Task - MAX31856 logging OFF\n");
            break;
        case MAX31856_TASK_COMMAND_READ_TC1:
        case MAX31856_TASK_COMMAND_READ_TC2:
        case MAX31856_TASK_COMMAND_READ_TC3: {
            const int tcIndex = cm.GetTaskCommand() - MAX31856_TASK_COMMAND_READ_TC1;
            if (_sensorReady[tcIndex] && _enableReading[tcIndex]) {
                const float tempC = _thermos[tcIndex]->ReadThermocoupleTempC();
                const uint8_t fault = _thermos[tcIndex]->GetFaultStatus();

                if (_enableLogging) {
                    const float tempUnits = _outputInC ? tempC : celsiusToFahrenheit(tempC);
                    SOAR_PRINT("MAX31856Task - TC%d temp: %.2f %s, fault: 0x%02X\n",
                               tcIndex + 1,
                               static_cast<double>(tempUnits),
                               _outputInC ? "C" : "F",
                               static_cast<unsigned int>(fault));
                }
            }
            break;
        }
        case MAX31856_TASK_COMMAND_READ_FORCE_TC1: {
            const int tcIndex = 0;
            if (_sensorReady[tcIndex]) {
                for (int sample = 0; sample < MAX31856_DEBUG_SAMPLES; ++sample) {
                    uint8_t ltcRaw[3] = {0, 0, 0};
                    _thermos[tcIndex]->GetMultipleRegisters(MAX31856_REG::LTCBH, 3, ltcRaw);
                    const float tempC = _thermos[tcIndex]->ReadThermocoupleTempC();
                    const uint8_t fault = _thermos[tcIndex]->GetFaultStatus();
                    const float tempUnits = _outputInC ? tempC : celsiusToFahrenheit(tempC);
                    SOAR_PRINT("MAX31856Task - TC1 sample %d LTCB: 0x%02X 0x%02X 0x%02X\n",
                               sample,
                               static_cast<unsigned int>(ltcRaw[0]),
                               static_cast<unsigned int>(ltcRaw[1]),
                               static_cast<unsigned int>(ltcRaw[2]));
                    SOAR_PRINT("MAX31856Task - TC1 sample %d temp: %.2f %s, fault: 0x%02X\n",
                               sample,
                               static_cast<double>(tempUnits),
                               _outputInC ? "C" : "F",
                               static_cast<unsigned int>(fault));
                    if (sample + 1 < MAX31856_DEBUG_SAMPLES) {
                        HAL_Delay(MAX31856_DEBUG_SAMPLE_DELAY_MS);
                    }
                }
            } else {
                SOAR_PRINT("MAX31856Task - TC1 force read skipped (not initialized)\n");
            }
            break;
        }
        case MAX31856_TASK_COMMAND_READ_REGS_TC1: {
            const int tcIndex = 0;
            if (_sensorReady[tcIndex]) {
                const uint8_t cr0 = _thermos[tcIndex]->GetCR0();
                const uint8_t cr1 = _thermos[tcIndex]->GetCR1();
                const uint8_t mask = _thermos[tcIndex]->GetMASK();
                const uint8_t sr = _thermos[tcIndex]->GetFaultStatus();
                SOAR_PRINT("MAX31856Task - TC1 regs: CR0=0x%02X CR1=0x%02X MASK=0x%02X SR=0x%02X\n",
                           static_cast<unsigned int>(cr0),
                           static_cast<unsigned int>(cr1),
                           static_cast<unsigned int>(mask),
                           static_cast<unsigned int>(sr));
            } else {
                SOAR_PRINT("MAX31856Task - TC1 regs skipped (not initialized)\n");
            }
            break;
        }
        case MAX31856_TASK_COMMAND_TOGGLE_CS_TC1: {
            const int tcIndex = 0;
            if (_sensorReady[tcIndex]) {
                HAL_GPIO_WritePin(TC1_cs_GPIO_Port, TC1_cs_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(TC1_cs_GPIO_Port, TC1_cs_Pin, GPIO_PIN_SET);
            } else {
                SOAR_PRINT("MAX31856Task - TC1 CS toggle skipped (not initialized)\n");
            }
            SOAR_PRINT("MAX31856Task - TC1 CS toggled\n");
            break;
        }
        case MAX31856_TASK_COMMAND_TOGGLE_TC1:
        case MAX31856_TASK_COMMAND_TOGGLE_TC2:
        case MAX31856_TASK_COMMAND_TOGGLE_TC3: {
            const int tcIndex = cm.GetTaskCommand() - MAX31856_TASK_COMMAND_TOGGLE_TC1;
            _enableReading[tcIndex] = !_enableReading[tcIndex];
            SOAR_PRINT("MAX31856Task - TC%d read %s\n",
                       tcIndex + 1,
                       _enableReading[tcIndex] ? "ON" : "OFF");
            break;
        }
        default:
            SOAR_PRINT("MAX31856Task - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("MAX31856Task - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    cm.Reset();
}

extern "C" void MAX31856Task_HandleDrdyInterrupt(uint16_t gpioPin)
{
    Command cmd(DATA_COMMAND, MAX31856_TASK_COMMAND_NONE);
    bool shouldSend = false;
    
    if (gpioPin == TC1_nReady_Pin) {
        cmd.SetTaskCommand(MAX31856_TASK_COMMAND_READ_TC1);
        shouldSend = true;
        gMax31856DrdyCounts[0] = gMax31856DrdyCounts[0] + 1;
        
    } else if (gpioPin == TC2_nReady_Pin) {
        cmd.SetTaskCommand(MAX31856_TASK_COMMAND_READ_TC2);
        shouldSend = true;
        gMax31856DrdyCounts[1] = gMax31856DrdyCounts[1] + 1;
        
    } else if (gpioPin == TC3_nReady_Pin) {
        cmd.SetTaskCommand(MAX31856_TASK_COMMAND_READ_TC3);
        shouldSend = true;
        
        gMax31856DrdyCounts[2] = gMax31856DrdyCounts[2] + 1;
    }
    
    if (shouldSend) {
        MAX31856Task::Inst().GetEventQueue()->SendFromISR(cmd);
    }
}
