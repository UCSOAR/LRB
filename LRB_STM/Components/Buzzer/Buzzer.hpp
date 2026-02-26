d
#ifndef __BUZZER_HPP
#define __BUZZER_HPP

#include "stm32g4xx_hal.h"

class Buzzer {
public:
  Buzzer(TIM_HandleTypeDef* htim, uint32_t channel)
    : _htim(htim), _channel(channel) {}

  void init() {
    // Initialization code for buzzer (if needed)
  }

  void task() {
    // Periodic logic for buzzer (to be implemented)
  }

  void beep(uint16_t freq, uint16_t duration_ms) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_Stop(_htim, _channel);
    // Compute period for desired frequency
    uint32_t timer_clock = HAL_RCC_GetPCLK2Freq();
    uint32_t period = (timer_clock / freq) - 1;
    __HAL_TIM_SET_AUTORELOAD(_htim, period);
    sConfigOC.Pulse = (period + 1) / 2; // 50% duty cycle
    HAL_TIM_PWM_ConfigChannel(_htim, &sConfigOC, _channel);
    HAL_TIM_PWM_Start(_htim, _channel);
    HAL_Delay(duration_ms);
    HAL_TIM_PWM_Stop(_htim, _channel);
  }

private:
  TIM_HandleTypeDef* _htim;
  uint32_t _channel;
};

#endif // __BUZZER_HPP
