/*
 * Task3.cpp
 *
 * Empty task implementation using SoarOS wrapper
 */

#include "Task3.hpp"

bool Task3::ReceiveData()
{
    return kUart_->ReceiveIT(&rxChar_, this);
}

/**
 * @brief Initializes Task3 with the RTOS scheduler
*/
void Task3::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Task3 twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)Task3::RunTask,
            (const char*)"Task3",
            (uint16_t)TASK3_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK3_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "Task3::InitTask() - xTaskCreate() failed");

    // Arm USART2 RX interrupt path via UARTDriver (LL based).
    SOAR_ASSERT(ReceiveData(), "Task3::InitTask() - ReceiveIT() failed");
}

void Task3::InterruptRxData(uint8_t errors)
{
    (void)errors;

    Command cm(DATA_COMMAND, TASK3_COMMAND_RX_COMPLETE);
    qEvtQueue->SendFromISR(cm);
}

/**
 * @brief Instance Run loop for Task3, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void Task3::Run(void * pvParams)
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
 * @brief HandleCommand handles any command passed to Task3 primary event queue.
 * @param cm Reference to the command object to handle
*/
void Task3::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        case TASK3_COMMAND_SEND_DEBUG:
#ifndef DISABLE_DEBUG
            kUart_->Transmit(cm.GetDataPointer(), cm.GetDataSize());
#endif
            break;
        case TASK3_COMMAND_RX_COMPLETE:
            // RX byte is captured by the UARTDriver callback path; no-op for now.
            break;
        default:
            SOAR_PRINT("Task3 - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("Task3 - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    // Reset allocated data
    cm.Reset();
}
