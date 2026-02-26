/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Debug_led1_Pin GPIO_PIN_13
#define Debug_led1_GPIO_Port GPIOC
#define Debug_led2_Pin GPIO_PIN_14
#define Debug_led2_GPIO_Port GPIOC
#define Debug_led3_Pin GPIO_PIN_15
#define Debug_led3_GPIO_Port GPIOC
#define RESET_Pin GPIO_PIN_10
#define RESET_GPIO_Port GPIOG
#define USART3_TX_FSB_Pin GPIO_PIN_10
#define USART3_TX_FSB_GPIO_Port GPIOB
#define USART3_RX_FSB_Pin GPIO_PIN_11
#define USART3_RX_FSB_GPIO_Port GPIOB
#define LC2_DRDY_Pin GPIO_PIN_12
#define LC2_DRDY_GPIO_Port GPIOB
#define LC1_DRDY_Pin GPIO_PIN_13
#define LC1_DRDY_GPIO_Port GPIOB
#define USART3_DE_FSB_Pin GPIO_PIN_14
#define USART3_DE_FSB_GPIO_Port GPIOB
#define TC3_nReady_Pin GPIO_PIN_8
#define TC3_nReady_GPIO_Port GPIOD
#define TC3_cs_Pin GPIO_PIN_9
#define TC3_cs_GPIO_Port GPIOD
#define TC3_nFAULT_Pin GPIO_PIN_10
#define TC3_nFAULT_GPIO_Port GPIOD
#define Ext_ind_led1_Pin GPIO_PIN_15
#define Ext_ind_led1_GPIO_Port GPIOA
#define Ext_ind_led2_Pin GPIO_PIN_10
#define Ext_ind_led2_GPIO_Port GPIOC
#define Ext_ind_led3_Pin GPIO_PIN_11
#define Ext_ind_led3_GPIO_Port GPIOC
#define Ext_ind_led4_Pin GPIO_PIN_12
#define Ext_ind_led4_GPIO_Port GPIOC
#define Ext_ind_led5_Pin GPIO_PIN_0
#define Ext_ind_led5_GPIO_Port GPIOD
#define TC1_nFault_Pin GPIO_PIN_1
#define TC1_nFault_GPIO_Port GPIOD
#define TC1_cs_Pin GPIO_PIN_2
#define TC1_cs_GPIO_Port GPIOD
#define TC1_nReady_Pin GPIO_PIN_4
#define TC1_nReady_GPIO_Port GPIOB
#define TC2_nFault_Pin GPIO_PIN_5
#define TC2_nFault_GPIO_Port GPIOB
#define TC2_cs_Pin GPIO_PIN_6
#define TC2_cs_GPIO_Port GPIOB
#define TC2_nReady_Pin GPIO_PIN_7
#define TC2_nReady_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
