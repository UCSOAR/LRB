/*
 * StartupLedTask.cpp
 *
 * Flashes debug LEDs at boot, then enables external LEDs
 */

#include "Task2.hpp"
#include "main.h"

namespace {
constexpr uint32_t TASK2_STARTUP_STEP_MS = 120;

inline void SetDebugLeds(bool on)
{
    if (on) {
        LL_GPIO_SetOutputPin(Debug_led1_GPIO_Port, Debug_led1_Pin);
        LL_GPIO_SetOutputPin(Debug_led2_GPIO_Port, Debug_led2_Pin);
        LL_GPIO_SetOutputPin(Debug_led3_GPIO_Port, Debug_led3_Pin);
    } else {
        LL_GPIO_ResetOutputPin(Debug_led1_GPIO_Port, Debug_led1_Pin);
        LL_GPIO_ResetOutputPin(Debug_led2_GPIO_Port, Debug_led2_Pin);
        LL_GPIO_ResetOutputPin(Debug_led3_GPIO_Port, Debug_led3_Pin);
    }
}

inline void SetExternalLeds(bool on)
{
    if (on) {
        LL_GPIO_SetOutputPin(MCU_Ext_KED_Ind1_GPIO_Port, MCU_Ext_KED_Ind1_Pin);
        LL_GPIO_SetOutputPin(MCU_Ext_KED_Ind2_GPIO_Port, MCU_Ext_KED_Ind2_Pin);
        LL_GPIO_SetOutputPin(MCU_Ext_KED_Ind3_GPIO_Port, MCU_Ext_KED_Ind3_Pin);
        LL_GPIO_SetOutputPin(MCU_Ext_KED_Ind4_GPIO_Port, MCU_Ext_KED_Ind4_Pin);
        LL_GPIO_SetOutputPin(MCU_Ext_KED_Ind5_GPIO_Port, MCU_Ext_KED_Ind5_Pin);
    } else {
        LL_GPIO_ResetOutputPin(MCU_Ext_KED_Ind1_GPIO_Port, MCU_Ext_KED_Ind1_Pin);
        LL_GPIO_ResetOutputPin(MCU_Ext_KED_Ind2_GPIO_Port, MCU_Ext_KED_Ind2_Pin);
        LL_GPIO_ResetOutputPin(MCU_Ext_KED_Ind3_GPIO_Port, MCU_Ext_KED_Ind3_Pin);
        LL_GPIO_ResetOutputPin(MCU_Ext_KED_Ind4_GPIO_Port, MCU_Ext_KED_Ind4_Pin);
        LL_GPIO_ResetOutputPin(MCU_Ext_KED_Ind5_GPIO_Port, MCU_Ext_KED_Ind5_Pin);
    }
}
}

/**
 * @brief Initializes StartupLedTask with the RTOS scheduler
*/
void StartupLedTask::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize StartupLedTask twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)StartupLedTask::RunTask,
            (const char*)"StartupLedTask",
            (uint16_t)TASK2_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK2_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "StartupLedTask::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Startup LED sequence and steady-state indication.
 */
void StartupLedTask::Run(void * pvParams)
{
    (void)pvParams;

    // Startup test pattern on debug LEDs.
    SetExternalLeds(false);
    SetDebugLeds(false);

    LL_GPIO_SetOutputPin(Debug_led1_GPIO_Port, Debug_led1_Pin);
    vTaskDelay(pdMS_TO_TICKS(TASK2_STARTUP_STEP_MS));
    LL_GPIO_SetOutputPin(Debug_led2_GPIO_Port, Debug_led2_Pin);
    vTaskDelay(pdMS_TO_TICKS(TASK2_STARTUP_STEP_MS));
    LL_GPIO_SetOutputPin(Debug_led3_GPIO_Port, Debug_led3_Pin);
    vTaskDelay(pdMS_TO_TICKS(TASK2_STARTUP_STEP_MS));

    // System started: debug LEDs off, external LEDs on.
    SetDebugLeds(false);
    SetExternalLeds(true);

    while(1) {
        Command cm;
        if (qEvtQueue->Receive(cm, 200)) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to StartupLedTask primary event queue.
 */
void StartupLedTask::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        default:
            SOAR_PRINT("StartupLedTask - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("StartupLedTask - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    cm.Reset();
}
