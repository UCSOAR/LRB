#include <AnemometerTask.hpp>

extern "C" {
#include "main.h"
extern ADC_HandleTypeDef hadc1;
}

namespace {
constexpr uint32_t ANEMOMETER_SAMPLE_PERIOD_MS = 100;

bool ReadAdcChannel(ADC_HandleTypeDef* hadc, uint32_t channel, uint32_t* outValue, uint32_t pollTimeoutMs = 50)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	// Use a longer sampling time to improve reliability on high source impedance sensors
	sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	HAL_StatusTypeDef st = HAL_ADC_ConfigChannel(hadc, &sConfig);
	if (st != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_ConfigChannel failed (ch=%lu) status=%d\n", (unsigned long)channel, (int)st);
		return false;
	}

	st = HAL_ADC_Start(hadc);
	if (st != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_Start failed status=%d\n", (int)st);
		return false;
	}

	const HAL_StatusTypeDef pollStatus = HAL_ADC_PollForConversion(hadc, pollTimeoutMs);
	if (pollStatus != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_PollForConversion timeout/status=%d\n", (int)pollStatus);
		(void)HAL_ADC_Stop(hadc);
		return false;
	}

	*outValue = HAL_ADC_GetValue(hadc);
	(void)HAL_ADC_Stop(hadc);
	return true;
}

// Differential read for ADC_CHANNEL_1 when ADC is configured in differential mode
bool ReadAdcDifferential(ADC_HandleTypeDef* hadc, uint32_t channel, int32_t* outValue, uint32_t pollTimeoutMs = 50)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel; // ADC_CHANNEL_1 represents the differential pair IN1-IN2
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
	sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	HAL_StatusTypeDef st = HAL_ADC_ConfigChannel(hadc, &sConfig);
	if (st != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_ConfigChannel(diff) failed status=%d\n", (int)st);
		return false;
	}

	st = HAL_ADC_Start(hadc);
	if (st != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_Start(diff) failed status=%d\n", (int)st);
		return false;
	}

	const HAL_StatusTypeDef pollStatus = HAL_ADC_PollForConversion(hadc, pollTimeoutMs);
	if (pollStatus != HAL_OK) {
		SOAR_PRINT("AnemometerTask - HAL_ADC_PollForConversion(diff) timeout/status=%d\n", (int)pollStatus);
		(void)HAL_ADC_Stop(hadc);
		return false;
	}

	// For differential mode the ADC result may be returned as signed two's complement in the ADC data register.
	// HAL_ADC_GetValue returns an unsigned value; reinterpret as signed with sign-extension based on ADC resolution.
	const uint32_t raw = HAL_ADC_GetValue(hadc);
	const int32_t signedVal = static_cast<int32_t>(raw);
	*outValue = signedVal;
	(void)HAL_ADC_Stop(hadc);
	return true;
}
}

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

void AnemometerTask::Run(void* pvParams)
{
	(void)pvParams;

	while (1) {
		Command cm;
		if (qEvtQueue->Receive(cm, ANEMOMETER_SAMPLE_PERIOD_MS)) {
			HandleCommand(cm);
			continue;
		}

		uint32_t adcIn1 = 0;
		uint32_t adcIn2 = 0;
		bool readIn1Ok = ReadAdcChannel(&hadc1, ADC_CHANNEL_1, &adcIn1, 100);
		bool readIn2Ok = ReadAdcChannel(&hadc1, ADC_CHANNEL_2, &adcIn2, 100);

		if (readIn1Ok && readIn2Ok) {
			const int32_t difference = static_cast<int32_t>(adcIn1) - static_cast<int32_t>(adcIn2);
			SOAR_PRINT("AnemometerTask - ADC1_IN1(raw)=%lu ADC1_IN2(raw)=%lu diff=%ld\n",
					   static_cast<unsigned long>(adcIn1),
					   static_cast<unsigned long>(adcIn2),
					   static_cast<long>(difference));
		} else {
			SOAR_PRINT("AnemometerTask - ADC single-ended read failed (IN1:%d IN2:%d)\n",
					   readIn1Ok ? 1 : 0,
					   readIn2Ok ? 1 : 0);

			// Attempt differential read fallback (reads ADC_CHANNEL_1 as differential IN1-IN2)
			int32_t diffRaw = 0;
			if (ReadAdcDifferential(&hadc1, ADC_CHANNEL_1, &diffRaw, 100)) {
				SOAR_PRINT("AnemometerTask - Differential fallback ADC1_IN1-IN2 (raw)=%ld\n", static_cast<long>(diffRaw));
			} else {
				SOAR_PRINT("AnemometerTask - Differential fallback read failed\n");
			}
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
