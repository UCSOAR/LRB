/**
 ******************************************************************************
 * File Name          : main_avionics.hpp
 * Description        : Header file for main_avionics.cpp, acts as an interface
 *between STM32CubeIDE and our application.
 ******************************************************************************
 */
#ifndef MAIN_SYSTEM_HPP_
#define MAIN_SYSTEM_HPP_

/* Includes
 * ----------------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_usart.h"

#include "../../SoarOS/Core/Inc/Mutex.hpp"


/* Interface Functions
 * ------------------------------------------------------------------*/
/* These functions act as our program's 'main' and any functions inside
 * CubeIDE's main --*/
void run_main();
void run_StartDefaultTask();

/* Global Functions
 * ------------------------------------------------------------------*/

/* Global Variable Interfaces
 * ------------------------------------------------------------------*/
/* All must be extern from main_avionics.cpp
 * -------------------------------------------------*/

/* Globally Accessible Drivers
 * ------------------------------------------------------------------*/
// UART Driver
class UARTDriver;
namespace Driver {
extern UARTDriver usart2;
extern UARTDriver usart3;
}
namespace UART {
constexpr UARTDriver* Debug = &Driver::usart2;
constexpr UARTDriver* FSB = &Driver::usart3;

void SetDebugRouteToFSB(bool enabled);
bool DebugRouteToFSB();
UARTDriver* GetDebugRouteSink();
}

/* System Handles
 * ------------------------------------------------------------------*/
extern CRC_HandleTypeDef hcrc;  // CRC - Hardware CRC System Handle

namespace SystemHandles {
constexpr CRC_HandleTypeDef* CRC_Handle = &hcrc;
}

#endif /* MAIN_SYSTEM_HPP_ */
