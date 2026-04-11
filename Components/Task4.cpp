/*
 * Task4.cpp
 *
 * Empty task implementation using SoarOS wrapper
 */

#include "Task4.hpp"

/**
 * @brief Initializes Task4 with the RTOS scheduler
*/
void Task4::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Task4 twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)Task4::RunTask,
            (const char*)"Task4",
            (uint16_t)TASK4_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK4_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "Task4::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance Run loop for Task4, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void Task4::Run(void * pvParams)
{
    while(1) {
        Command cm;

        // Wait forever for a command
        qEvtQueue->ReceiveWait(cm);

        // Process the command
        HandleCommand(cm);
    }
}

/**
 * @brief HandleCommand handles any command passed to Task4 primary event queue.
 * @param cm Reference to the command object to handle
*/
void Task4::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        default:
            SOAR_PRINT("Task4 - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("Task4 - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    // Reset allocated data
    cm.Reset();
}
