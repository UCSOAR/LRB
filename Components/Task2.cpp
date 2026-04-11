/*
 * Task2.cpp
 *
 * Empty task implementation using SoarOS wrapper
 */

#include "Task2.hpp"
#include "main.h"

/**
 * @brief Initializes Task2 with the RTOS scheduler
*/
void Task2::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Task2 twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)Task2::RunTask,
            (const char*)"Task2",
            (uint16_t)TASK2_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK2_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "Task2::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance Run loop for Task2, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void Task2::Run(void * pvParams)
{
    // Initialize debug LEDs to a known OFF state.
    LL_GPIO_ResetOutputPin(Debug_led1_GPIO_Port, Debug_led1_Pin);
    LL_GPIO_ResetOutputPin(Debug_led2_GPIO_Port, Debug_led2_Pin);
    LL_GPIO_ResetOutputPin(Debug_led3_GPIO_Port, Debug_led3_Pin);

    while(1) {
        Command cm;
        if (qEvtQueue->ReceiveWait(cm, pdMS_TO_TICKS(500))) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to Task2 primary event queue.
 * @param cm Reference to the command object to handle
*/
void Task2::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        default:
            SOAR_PRINT("Task2 - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("Task2 - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    // Reset allocated data
    cm.Reset();
}
