#include <AnemometerTask.hpp>

#include <cstdint>

extern "C" {
#include "main.h"
}

namespace {
constexpr uint32_t ANEMOMETER_WINDOW_MS = 1000;  // 1 second sample window
volatile uint32_t gAnemometerPulseCount = 0;
}

/**
 * @brief External interrupt callback for anemometer pulse (PA0).
 * Called on FALLING edge (LOW pulse from NPN sensor).
 */
/*
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_0) {
		gAnemometerPulseCount++;
	}
}
*/
void AnemometerTask::InitTask()
{
	SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize AnemometerTask twice");

	BaseType_t rtValue =
		xTaskCreate((TaskFunction_t)AnemometerTask::RunTask,
			(const char*)"AnemometerTask",
			(uint16_t)TASK1_STACK_DEPTH_WORDS,
			(void*)this,
			(UBaseType_t)TASK1_RTOS_PRIORITY,
			(TaskHandle_t*)&rtTaskHandle);

	SOAR_ASSERT(rtValue == pdPASS, "AnemometerTask::InitTask() - xTaskCreate() failed");
}

void AnemometerTask::SetTaskActive(bool enabled)
{
	_enableReading = enabled;

	if (enabled) {
		if (rtTaskHandle != nullptr) {
			vTaskResume(rtTaskHandle);
		}
		SOAR_PRINT("AnemometerTask - read ON\n");
	} else {
		SOAR_PRINT("AnemometerTask - read OFF\n");
		if (rtTaskHandle != nullptr) {
			vTaskSuspend(rtTaskHandle);
		}
	}
}

void AnemometerTask::ToggleTaskActive()
{
	SetTaskActive(!_enableReading);
}

void AnemometerTask::PrintStatus()
{
	SOAR_PRINT("AnemometerTask - status read=%s pulses=%lu pin=%d\n",
			   _enableReading ? "ON" : "OFF",
			   static_cast<unsigned long>(gAnemometerPulseCount),
			   (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1 : 0);
}

void AnemometerTask::Run(void* pvParams)
{
	(void)pvParams;

	SOAR_PRINT("AnemometerTask - Started, reading NPN pulses on PA0\n");

	unsigned long lastWindowTick = xTaskGetTickCount();

	while (1) {
		Command cm;
		if (qEvtQueue->Receive(cm, ANEMOMETER_WINDOW_MS)) {
			HandleCommand(cm);
			continue;
		}

		if (!_enableReading) {
			continue;
		}

		const unsigned long now = xTaskGetTickCount();

		// Every 1 second (ANEMOMETER_WINDOW_MS ticks), calculate and print wind speed
		if ((now - lastWindowTick) >= pdMS_TO_TICKS(ANEMOMETER_WINDOW_MS)) {
			lastWindowTick = now;

			// Disable interrupts to safely read pulse count
			taskENTER_CRITICAL();
			const uint32_t pulseCount = gAnemometerPulseCount;
			gAnemometerPulseCount = 0;
			taskEXIT_CRITICAL();

			// Wind speed calculation:
			// - 20 pulses per revolution = 1.75 m/s
			// - So: windSpeed_cm_per_sec = pulseCount * 8.75
			const uint32_t windSpeedCmS = pulseCount * 875 / 100;  // pulseCount * 8.75
			const uint32_t windSpeedMmS = (pulseCount * 875) % 100;  // fractional part
			const uint32_t windSpeedMS = windSpeedCmS / 100;  // m/s (whole)
			const uint32_t windSpeedMF = (windSpeedCmS % 100) * 10;  // m/s (fractional: centimeters * 10 = millimeters/second)

			SOAR_PRINT("AnemometerTask - Pulses=%lu WindSpeed=%lu.%02lu cm/s = %lu.%02lu m/s\n",
					   pulseCount,
					   windSpeedCmS, windSpeedMmS,
					   windSpeedMS, windSpeedMF);
		}
	}
}

void AnemometerTask::HandleCommand(Command& cm)
{
	switch (cm.GetCommand()) {
	case DATA_COMMAND:
		SOAR_PRINT("AnemometerTask - Unsupported DATA_COMMAND {%d}\n", cm.GetTaskCommand());
		break;
	default:
		SOAR_PRINT("AnemometerTask - Unsupported Command {%d}\n", cm.GetCommand());
		break;
	}

	cm.Reset();
}
