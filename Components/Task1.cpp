/*
 * Task1.cpp
 *
 * Empty task implementation using SoarOS wrapper
 */

#include "Task1.hpp"
#include "main.h"

extern "C" {
extern TIM_HandleTypeDef htim1;
}

constexpr uint32_t TASK1_CHIRP_PERIOD_MS = 60000;
constexpr uint32_t TASK1_CHIRP_ON_TIME_MS = 150;

/**
 * @brief Initializes Task1 with the RTOS scheduler
*/
void Task1::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize Task1 twice");

    // Start the task
    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)Task1::RunTask,
            (const char*)"Task1",
            (uint16_t)TASK1_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK1_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    // Ensure creation succeeded
    SOAR_ASSERT(rtValue == pdPASS, "Task1::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Instance Run loop for Task1, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
*/
void Task1::Run(void * pvParams)
{
    uint32_t lastChirpTick = xTaskGetTickCount();

    while(1) {
        const uint32_t now = xTaskGetTickCount();
        if ((now - lastChirpTick) >= pdMS_TO_TICKS(TASK1_CHIRP_PERIOD_MS)) {
            lastChirpTick = now;

            // 50% duty cycle on TIM1 CH1 for a short chirp.
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (htim1.Init.Period + 1U) / 2U);
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            vTaskDelay(pdMS_TO_TICKS(TASK1_CHIRP_ON_TIME_MS));
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
        }

        Command cm;
        if (qEvtQueue->ReceiveWait(cm, pdMS_TO_TICKS(100))) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to Task1 primary event queue.
 * @param cm Reference to the command object to handle
*/
void Task1::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        default:
            SOAR_PRINT("Task1 - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("Task1 - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    // Reset allocated data
    cm.Reset();
}
