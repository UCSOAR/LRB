/*
 * BuzzerTask.cpp
 *
 * Plays a startup chirp and supports a watchdog/fault alert pattern.
 */

#include "BuzzerTask.hpp"
#include "main.h"

extern "C" {
extern TIM_HandleTypeDef htim1;
}

namespace {
constexpr unsigned long BUZZER_LOOP_TIMEOUT_MS = 250;
constexpr unsigned long BUZZER_INIT_CHIRP_ON_MS = 100;
constexpr unsigned long BUZZER_WATCHDOG_ON_MS = 120;
constexpr unsigned long BUZZER_WATCHDOG_OFF_MS = 90;
constexpr unsigned char BUZZER_WATCHDOG_CHIRP_COUNT = 3;

constexpr unsigned long BUZZER_PWM_FREQ_HZ = 3000;
constexpr unsigned long BUZZER_PWM_DUTY_PERCENT = 50;

inline bool StartBuzzerPwm()
{
    const unsigned long timerClockHz = HAL_RCC_GetPCLK2Freq();
    if (timerClockHz == 0U || BUZZER_PWM_FREQ_HZ == 0U || BUZZER_PWM_DUTY_PERCENT > 100U) {
        return false;
    }

    const unsigned long periodCounts = (timerClockHz / BUZZER_PWM_FREQ_HZ);
    if (periodCounts < 2U) {
        return false;
    }

    const unsigned long arr = periodCounts - 1U;
    const unsigned long compare = (arr * BUZZER_PWM_DUTY_PERCENT) / 100U;

    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);

    return (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) == HAL_OK);
}

inline void StopBuzzerPwm()
{
    (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}
}

/**
 * @brief Initializes BuzzerTask with the RTOS scheduler
*/
void BuzzerTask::InitTask()
{
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize BuzzerTask twice");

    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)BuzzerTask::RunTask,
            (const char*)"BuzzerTask",
            (uint16_t)TASK_BUZZER_STACK_DEPTH_WORDS,
            (void*)this,
            (UBaseType_t)TASK_BUZZER_RTOS_PRIORITY,
            (TaskHandle_t*)&rtTaskHandle);

    SOAR_ASSERT(rtValue == pdPASS, "BuzzerTask::InitTask() - xTaskCreate() failed");
}

/**
 * @brief Queue watchdog/fault buzzer pattern for async playback.
 */
void BuzzerTask::RequestWatchdogAlert()
{
    Command cm(DATA_COMMAND, BUZZER_TASK_COMMAND_PLAY_WATCHDOG_ALERT);
    qEvtQueue->Send(cm);
}

/**
 * @brief Task loop.
 */
void BuzzerTask::Run(void* pvParams)
{
    (void)pvParams;

    PlayInitChirp();

    while (1) {
        Command cm;
        if (qEvtQueue->Receive(cm, BUZZER_LOOP_TIMEOUT_MS)) {
            HandleCommand(cm);
        }
    }
}

/**
 * @brief HandleCommand handles any command passed to BuzzerTask primary event queue.
 */
void BuzzerTask::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {
    case DATA_COMMAND: {
        switch (cm.GetTaskCommand()) {
        case BUZZER_TASK_COMMAND_PLAY_WATCHDOG_ALERT:
            PlayWatchdogAlertPattern();
            break;
        case BUZZER_TASK_COMMAND_PLAY_BOMB_SOUND:
            BombHasBeenPlanted();
            break;
        default:
            SOAR_PRINT("BuzzerTask - Received Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
            break;
        }
        break;
    }
    default:
        SOAR_PRINT("BuzzerTask - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    cm.Reset();
}

void BuzzerTask::BombHasBeenPlanted()
{
    // CS:GO bomb planting sound: escalating beeps for 5 seconds, then detonation slide
    
    const unsigned long BEEP_FREQ_LOW = 1500;
    // const unsigned long BEEP_FREQ_HIGH = 2000;
    const unsigned long DETONATE_FREQ = 2500;
    
    // Phase 1: Escalating beeps (5 seconds total)
    unsigned long beepOnMs = 100;
    unsigned long beepOffMs = 400;
    unsigned long elapsedMs = 0;
    unsigned long totalPhase1Ms = 5000;
    
    while (elapsedMs < totalPhase1Ms) {
        // Play beep at low frequency
        const unsigned long timerClockHz = HAL_RCC_GetPCLK2Freq();
        const unsigned long periodCounts = (timerClockHz / BEEP_FREQ_LOW);
        const unsigned long arr = periodCounts - 1U;
        const unsigned long compare = (arr * BUZZER_PWM_DUTY_PERCENT) / 100U;
        
        __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare);
        __HAL_TIM_SET_COUNTER(&htim1, 0U);
        (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        
        vTaskDelay(pdMS_TO_TICKS(beepOnMs));
        StopBuzzerPwm();
        
        elapsedMs += beepOnMs;
        if (elapsedMs >= totalPhase1Ms) break;
        
        vTaskDelay(pdMS_TO_TICKS(beepOffMs));
        elapsedMs += beepOffMs;
        
        // Accelerate: reduce off-time, keep on-time consistent
        if (beepOffMs > 80) beepOffMs -= 25;
        if (beepOffMs < 80) beepOffMs = 80;
    }
    
    // Phase 2: Detonation - rapid high-frequency burst (1 second)
    const unsigned long detonate_timerClockHz = HAL_RCC_GetPCLK2Freq();
    const unsigned long detonate_periodCounts = (detonate_timerClockHz / DETONATE_FREQ);
    const unsigned long detonate_arr = detonate_periodCounts - 1U;
    const unsigned long detonate_compare = (detonate_arr * BUZZER_PWM_DUTY_PERCENT) / 100U;
    
    __HAL_TIM_SET_AUTORELOAD(&htim1, detonate_arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, detonate_compare);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    StopBuzzerPwm();
}

void BuzzerTask::PlayInitChirp()
{
    if (!StartBuzzerPwm()) {
        SOAR_PRINT("BuzzerTask - TIM1 PWM start failed for init chirp\n");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(BUZZER_INIT_CHIRP_ON_MS));
    StopBuzzerPwm();
}

void BuzzerTask::PlayWatchdogAlertPattern()
{
    for (unsigned char i = 0; i < BUZZER_WATCHDOG_CHIRP_COUNT; ++i) {
        if (!StartBuzzerPwm()) {
            SOAR_PRINT("BuzzerTask - TIM1 PWM start failed for watchdog alert\n");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WATCHDOG_ON_MS));
        StopBuzzerPwm();
        vTaskDelay(pdMS_TO_TICKS(BUZZER_WATCHDOG_OFF_MS));
    }
}
