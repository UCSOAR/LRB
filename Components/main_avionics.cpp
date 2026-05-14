/**
 ******************************************************************************
 * File Name          : main_avionics.cpp
 * Description        : This file acts as an interface supporting CubeIDE
 Codegen while having a clean interface for development.
 ******************************************************************************
*/
/* Includes -----------------------------------------------------------------*/
#include <MAX31856Task.hpp>
#include <AnemometerTask.hpp>
#include <StartupLedTask.hpp>
#include <BuzzerTask.hpp>
#include <NAU7802Task.hpp>
#include "SystemDefines.hpp"
#include "DebugTask.hpp"

#include "../SoarOS/CubeTask.hpp"
#include "../SoarOS/Drivers/Inc/UARTDriver.hpp"

#include "TopTask.hpp"

#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"

// Tasks

/* Drivers ------------------------------------------------------------------*/
namespace Driver
{
  UARTDriver usart2(USART2);
  UARTDriver usart3(USART3);
}

namespace UART
{
namespace
{
  volatile bool gRouteDebugToFSB = DEBUG_ROUTE_TO_FSB_DEFAULT;
}

void SetDebugRouteToFSB(bool enabled)
{
  gRouteDebugToFSB = enabled;
}

bool DebugRouteToFSB()
{
  return gRouteDebugToFSB;
}

UARTDriver* GetDebugRouteSink()
{
  return DebugRouteToFSB() ? FSB : Debug;
}
}

/* Interface Functions
 * ------------------------------------------------------------*/
/**
 * @brief Main function interface, called inside main.cpp before os
 * initialization takes place.
 */
void run_main()
{
  UART::SetDebugRouteToFSB(DEBUG_ROUTE_TO_FSB_DEFAULT);

  // Init Tasks
  CubeTask::Inst().InitTask();
  DebugTask::Inst().InitTask();

  NAU7802Task::Inst().InitTask();
  MAX31856Task::Inst().InitTask();
  AnemometerTask::Inst().InitTask();
  StartupLedTask::Inst().InitTask();
  BuzzerTask::Inst().InitTask();



#if (configGENERATE_RUN_TIME_STATS == 1)
  TopTask::Inst().InitTask();
#endif

  // Print System Boot Info : Warning, don't queue more than 10 prints before
  // scheduler starts
  SOAR_PRINT("\n-- CUBE SYSTEM --\n");
  SOAR_PRINT(
      "System Reset Reason: [TODO]\n"); // TODO: System reset reason can be
                                        // implemented via. Flash storage
  SOAR_PRINT("Current System Free Heap: %d Bytes\n", xPortGetFreeHeapSize());
  SOAR_PRINT("Lowest Ever Free Heap: %d Bytes\n\n",
             xPortGetMinimumEverFreeHeapSize());

  // Start the Scheduler
  // Guidelines:
  // - Be CAREFUL with race conditions after osKernelStart
  // - All uses of new and delete should be closely monitored after this point
  osKernelStart();

  // Should never reach here
  SOAR_ASSERT(false, "osKernelStart() failed");

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_NVIC_SystemReset();
  }
}
